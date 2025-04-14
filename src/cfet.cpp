#include "cfet.h"

#include <omp.h>
#include <sys/stat.h>  // Linux/Unix 系統用於創建資料夾

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "z3++.h"

namespace fs = std::filesystem;

int createFolder(const std::string &folderName) {
#ifdef _WIN32
    return _mkdir(folderName.c_str());  // Windows 系統
#else
    return mkdir(folderName.c_str(), 0777);  // Linux/Unix 系統
#endif
}

Shape::Shape() {
    num_row = 0;
    num_finger = 0;
    id = 0;
}
Transistor::Transistor() {}

Signal::Signal() {}

Pshape::Pshape() {}

Node::Node() { fill = 0; }

Lambda::Lambda() {}

std::string cell_name;
std::vector<std::string> io_pins;
std::vector<Signal *> outputs;
std::vector<Signal *> inputs;
std::vector<Transistor *> trs;
std::vector<Transistor *> pmos;
std::vector<Transistor *> nmos;
std::unordered_map<std::string, Transistor *> tr_dict;
std::unordered_map<std::string, Signal *> signals;
std::unordered_map<Transistor *, Transistor *> tr_pairs;
std::unordered_map<Transistor *, std::unordered_map<Shape *, std::vector<Lambda *>>> phi;
std::unordered_map<Transistor *, std::unordered_map<Shape *, std::vector<Lambda *>>> phi_merged;
std::unordered_map<std::string, Shape *> CFETShapes;
std::unordered_map<Transistor *, std::vector<Lambda *>> multi_row_configs;
int tr_size_sum = 0;
std::unordered_map<int, Lambda *> lambda_dict;
int *ds_dict;
int lamb_id = 0;
bool compareGroupPair(const Group_pair *a, const Group_pair *b) {
    return a->common > b->common;  // 降序
}

bool comparePshapeHSP(const Pshape *a, const Pshape *b) { return a->hsp < b->hsp; }

bool comparePshapeHSP_descending(const Pshape *a, const Pshape *b) { return a->hsp > b->hsp; }

std::vector<std::set<Signal *>> findIntersectingElements(const std::vector<std::set<Signal *>> &sets) {
    // 用於儲存每個集合中有交集的元素
    std::vector<std::set<Signal *>> intersectingElements(sets.size());
    std::vector<std::set<Signal *>> results;

    for (size_t i = 0; i < sets.size(); ++i) {
        for (size_t j = 0; j < sets.size(); ++j) {
            if (i != j) {
                // 計算集合 i 和集合 j 的交集
                std::set<Signal *> intersection;
                std::set_intersection(sets[i].begin(), sets[i].end(), sets[j].begin(), sets[j].end(), std::inserter(intersection, intersection.begin()));

                // 將交集元素加入到集合 i 的交集結果中
                intersectingElements[i].insert(intersection.begin(), intersection.end());
            }
        }
    }

    // 輸出每個集合中有交集的元素
    for (size_t i = 0; i < intersectingElements.size(); ++i) {
        results.push_back(intersectingElements[i]);
        std::cout << "Set " << i + 1 << " intersecting elements: ";
        if (intersectingElements[i].empty()) {
            std::cout << "None";
        } else {
            for (Signal *elem : intersectingElements[i]) {
                std::cout << elem->name << " ";
            }
        }
        std::cout << std::endl;
    }
    return results;
}

void print_pshape(Pshape *pshape) {
    // print transistor name
    for (int i = 0; i < pshape->height; i++) {
        for (int j = 0; j < pshape->width; j++) {
            if (pshape->multirow_tr_permutation_up[i][j] == nullptr) {
                std::cout << "     ";
            } else {
                std::cout << std::left << std::setw(7) << pshape->multirow_tr_permutation_up[i][j]->name << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "--" << std::endl;
    for (int i = 0; i < pshape->height; i++) {
        for (int j = 0; j < pshape->width; j++) {
            if (pshape->multirow_tr_permutation_down[i][j] == nullptr) {
                std::cout << "     ";
            } else {
                std::cout << std::left << std::setw(7) << pshape->multirow_tr_permutation_down[i][j]->name << " ";
            }
        }
        std::cout << std::endl;
    }
    // print active
    for (int i = 0; i < pshape->height; i++) {
        for (int j = 0; j < pshape->width; j++) {
            Transistor *tr_n = pshape->multirow_tr_permutation_up[i][j];
            switch (pshape->multirow_tr_shape_up[i][j]) {
                case 0:
                    std::cout << std::left << std::setw(7) << tr_n->drain->name << " " << std::left << std::setw(7) << tr_n->gate->name << " " << std::left
                              << std::setw(7) << tr_n->source->name << " ";
                    break;
                case 1:
                    std::cout << std::left << std::setw(7) << tr_n->source->name << " " << std::left << std::setw(7) << tr_n->gate->name << " " << std::left
                              << std::setw(7) << tr_n->drain->name << " ";
                    break;
                case 2:
                    std::cout << "------------------------";
                    break;
            }
        }
        std::cout << std::endl;
    }
    std::cout << "--" << std::endl;
    for (int i = 0; i < pshape->height; i++) {
        for (int j = 0; j < pshape->width; j++) {
            Transistor *tr_p = pshape->multirow_tr_permutation_down[i][j];
            switch (pshape->multirow_tr_shape_down[i][j]) {
                case 0:
                    std::cout << std::left << std::setw(7) << tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left
                              << std::setw(7) << tr_p->source->name << " ";
                    break;
                case 1:
                    std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left
                              << std::setw(7) << tr_p->drain->name << " ";
                    break;
                case 2:
                    std::cout << "------------------------";
                    break;
            }
        }
        std::cout << std::endl;
    }
    // for (int i = 0; i < pshape->height; i++) {
    //     for (int j = 0; j < pshape->width; j++) {
    //         std::cout << pshape->multirow_tr_shape_up[i][j];
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "--" << std::endl;
    // for (int i = 0; i < pshape->height; i++) {
    //     for (int j = 0; j < pshape->width; j++) {
    //         std::cout << pshape->multirow_tr_shape_down[i][j];
    //     }
    //     std::cout << std::endl;
    // }
    std::cout << "vdd pgr: " << std::endl;
    for (int i = 0; i < pshape->multirow_pgr_blocked_vdd.size(); i++) {
        std::cout << pshape->multirow_pgr_blocked_vdd[i] << "       ";
    }
    std::cout << std::endl;
    std::cout << "vss pgr: " << std::endl;
    for (int i = 0; i < pshape->multirow_pgr_blocked_vss.size(); i++) {
        std::cout << pshape->multirow_pgr_blocked_vss[i] << "       ";
    }
    std::cout << std::endl;
}

void print_signal_permutation(std::vector<Pshape *> pshape_vec) {
    int layout_width = 0;
    for (int i = 0; i < pshape_vec.size(); i++) {
        if (pshape_vec[i]->multirow_tr_permutation_up.size() > layout_width) {
            layout_width = pshape_vec[i]->multirow_tr_permutation_up[0].size();
        }
    }
    std::cout << "signal_permutation: " << std::endl;
    for (int i = 0; i < pshape_vec.size(); i++) {
        Pshape *pshape = pshape_vec[i];
        for (int j = 0; j < pshape->multirow_tr_permutation_up[0].size(); j++) {
            pshape->multirow_signal_permutation_up.assign(pshape->height, std::vector<Signal *>(layout_width * 2 + 3, nullptr));
        }
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                Transistor *tr_n = pshape->multirow_tr_permutation_up[i][j];
                switch (pshape->multirow_tr_shape_up[i][j]) {
                    case 0:
                        pshape->multirow_signal_permutation_up[i][j * 2] = tr_n->drain;
                        pshape->multirow_signal_permutation_up[i][j * 2 + 1] = tr_n->gate;
                        pshape->multirow_signal_permutation_up[i][j * 2 + 2] = tr_n->source;
                        break;
                    case 1:
                        pshape->multirow_signal_permutation_up[i][j * 2] = tr_n->source;
                        pshape->multirow_signal_permutation_up[i][j * 2 + 1] = tr_n->gate;
                        pshape->multirow_signal_permutation_up[i][j * 2 + 2] = tr_n->drain;
                        break;
                    case 2:
                        pshape->multirow_signal_permutation_up[i][j * 2] = nullptr;
                        pshape->multirow_signal_permutation_up[i][j * 2 + 1] = nullptr;
                        pshape->multirow_signal_permutation_up[i][j * 2 + 2] = nullptr;
                        break;
                }
            }
            std::cout << std::endl;
        }
        pshape->multirow_signal_permutation_down.assign(pshape->height, std::vector<Signal *>(layout_width * 2 + 3, nullptr));
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                Transistor *tr_p = pshape->multirow_tr_permutation_down[i][j];
                switch (pshape->multirow_tr_shape_down[i][j]) {
                    case 0:
                        pshape->multirow_signal_permutation_down[i][j * 2] = tr_p->drain;
                        pshape->multirow_signal_permutation_down[i][j * 2 + 1] = tr_p->gate;
                        pshape->multirow_signal_permutation_down[i][j * 2 + 2] = tr_p->source;
                        break;
                    case 1:
                        pshape->multirow_signal_permutation_down[i][j * 2] = tr_p->source;
                        pshape->multirow_signal_permutation_down[i][j * 2 + 1] = tr_p->gate;
                        pshape->multirow_signal_permutation_down[i][j * 2 + 2] = tr_p->drain;
                        break;
                    case 2:
                        pshape->multirow_signal_permutation_down[i][j * 2] = nullptr;
                        pshape->multirow_signal_permutation_down[i][j * 2 + 1] = nullptr;
                        pshape->multirow_signal_permutation_down[i][j * 2 + 2] = nullptr;
                        break;
                }
            }
            std::cout << std::endl;
        }
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->multirow_signal_permutation_up[0].size(); j++) {
                Signal *sig = pshape->multirow_signal_permutation_up[0][j];
                if (sig == nullptr) {
                    std::cout << "Null    ";
                } else {
                    std::cout << std::left << std::setw(7) << sig->name << " ";
                }
            }
        }
        std::cout << std::endl;
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->multirow_signal_permutation_down[0].size(); j++) {
                Signal *sig = pshape->multirow_signal_permutation_down[0][j];
                if (sig == nullptr) {
                    std::cout << "Null    ";
                } else {
                    std::cout << std::left << std::setw(7) << sig->name << " ";
                }
            }
        }
        std::cout << std::endl;
    }
}

std::vector<std::vector<int>> calculate_available_track_case(std::vector<Pshape *> pshape_vec) {
    std::vector<std::vector<int>> available_track_case;
    int layout_width = 0;
    for (int i = 0; i < pshape_vec.size(); i++) {
        if (pshape_vec[i]->multirow_tr_permutation_up.size() > layout_width) {
            layout_width = pshape_vec[i]->multirow_tr_permutation_up[0].size();
        }
    }
    available_track_case.assign(pshape_vec.size(), std::vector<int>(layout_width * 2 + 3, 0));
    for (int i = 0; i < pshape_vec.size(); i++) {
        Pshape *pshape = pshape_vec[i];
        for (int j = 0; j < pshape->multirow_signal_permutation_up[0].size(); j++) {
            Signal *sig_n = pshape->multirow_signal_permutation_up[0][j];
            Signal *sig_p = pshape->multirow_signal_permutation_down[0][j];
            if (sig_n == nullptr && sig_p == nullptr) {
                available_track_case[i][j] = 0;
            } else if (sig_n->name == sig_p->name && sig_n->name != "VSS" && sig_p->name != "VDD") {
                available_track_case[i][j] = 1;
            } else if (sig_n->name != "VSS" && sig_p->name == "VDD") {
                available_track_case[i][j] = 2;
            } else if (sig_n->name != "VSS" && sig_p->name != "VDD") {
                available_track_case[i][j] = 3;
            } else if (sig_n->name == "VSS" && sig_p->name != "VDD") {
                available_track_case[i][j] = 4;
            } else if (sig_n->name == "VSS" && sig_p->name == "VDD") {
                available_track_case[i][j] = 5;
            }
        }
    }
    for (int i = 0; i < available_track_case.size(); i++) {
        for (int j = 0; j < available_track_case[i].size(); j++) {
            std::cout << available_track_case[i][j];
        }
        std::cout << std::endl;
    }
    return available_track_case;
}

bool via_rule_violation(std::vector<std::vector<int>> available_track_case) {
    z3::context ctx;
    z3::expr_vector bool_vars(ctx);
    int rows = available_track_case.size() * 4;
    int cols = available_track_case[0].size();
    std::vector<std::vector<z3::expr>> matrix(rows, std::vector<z3::expr>(cols, ctx.bool_val(false)));
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            std::string var_name = "b_" + std::to_string(i) + "_" + std::to_string(j);
            matrix[i][j] = ctx.bool_const(var_name.c_str());
        }
    }
    z3::solver solver(ctx);

    // 加入約束：相鄰的方格不能同時為 true
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i > 0)  // 上方相鄰
                solver.add(!(matrix[i][j] && matrix[i - 1][j]));
            if (i < rows - 1)  // 下方相鄰
                solver.add(!(matrix[i][j] && matrix[i + 1][j]));
            if (j > 0)  // 左方相鄰
                solver.add(!(matrix[i][j] && matrix[i][j - 1]));
            if (j < cols - 1)  // 右方相鄰
                solver.add(!(matrix[i][j] && matrix[i][j + 1]));
        }
    }
    for (int i = 0; i < available_track_case.size(); i++) {
        for (int j = 0; j < available_track_case[i].size(); j++) {
            // for (int j = 0; j < 3; j++) {
            int track_case = available_track_case[i][j];
            switch (track_case) {
                case 0:
                    break;
                case 1:
                    solver.add(z3::ite(matrix[4 * i][j], ctx.int_val(1), ctx.int_val(0)) + z3::ite(matrix[4 * i + 1][j], ctx.int_val(1), ctx.int_val(0)) +
                                   z3::ite(matrix[4 * i + 2][j], ctx.int_val(1), ctx.int_val(0)) +
                                   z3::ite(matrix[4 * i + 3][j], ctx.int_val(1), ctx.int_val(0)) ==
                               ctx.int_val(1));

                    break;
                case 2:
                    solver.add(z3::ite(matrix[4 * i][j], ctx.int_val(1), ctx.int_val(0)) + z3::ite(matrix[4 * i + 1][j], ctx.int_val(1), ctx.int_val(0)) +
                                   z3::ite(matrix[4 * i + 2][j], ctx.int_val(1), ctx.int_val(0)) +
                                   z3::ite(matrix[4 * i + 3][j], ctx.int_val(1), ctx.int_val(0)) ==
                               ctx.int_val(1));

                    break;
                case 3: {
                    z3::expr p1 = matrix[4 * i + 1][j] && matrix[4 * i + 3][j];
                    z3::expr p2 = matrix[4 * i][j] && matrix[4 * i + 3][j];
                    z3::expr p3 = matrix[4 * i + 2][j] && matrix[4 * i][j];
                    z3::expr p4 = matrix[4 * i + 3][j] && matrix[4 * i][j];
                    solver.add(z3::ite(p1, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p2, ctx.int_val(1), ctx.int_val(0)) +
                                   z3::ite(p3, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p4, ctx.int_val(1), ctx.int_val(0)) ==
                               ctx.int_val(1));
                    break;
                }
                case 4: {
                    if (i % 2 == 0) {
                        solver.add(!matrix[4 * i][j] && !matrix[4 * i + 1][j] && !matrix[4 * i + 2][j] && matrix[4 * i + 3][j]);
                    } else {
                        solver.add(matrix[4 * i][j] && !matrix[4 * i + 1][j] && !matrix[4 * i + 2][j] && !matrix[4 * i + 3][j]);
                    }
                    break;
                }
                case 5: {
                    solver.add(!matrix[4 * i][j] && !matrix[4 * i + 1][j] && !matrix[4 * i + 2][j] && !matrix[4 * i + 3][j]);
                    break;
                }
            }
        }
    }
    if (solver.check() == z3::sat) {
        std::cout << "SAT" << std::endl;
        z3::model m = solver.get_model();
        for (int i = 0; i < matrix.size(); i++) {
            for (int j = 0; j < matrix[i].size(); j++) {
                std::cout << (m.eval(matrix[i][j]).bool_value() == Z3_L_TRUE ? "1 " : "0 ");
            }
            std::cout << std::endl;
        }
        return false;
    } else {
        std::cout << "UNSAT" << std::endl;
        return true;
    }
}

std::vector<Pshape *> single_row_merging(std::vector<Pshape *> pshape_initial_vec) {
    std::vector<std::vector<Pshape *>> permutations;
    std::vector<Pshape *> results;
    // std::sort(pshape_vec.begin(), pshape_vec.end(), comparePshapeHSP_descending);
    std::sort(pshape_initial_vec.begin(), pshape_initial_vec.end());

    do {
        permutations.push_back(pshape_initial_vec);
    } while (std::next_permutation(pshape_initial_vec.begin(), pshape_initial_vec.end()));

    for (int i = 0; i < permutations.size(); i++) {
        auto pshape_vec = permutations[i];
        Pshape *pshape_0 = pshape_vec[0];
        Pshape *pshape = new Pshape();
        pshape->multirow_tr_permutation_up = pshape_0->multirow_tr_permutation_up;
        pshape->multirow_tr_permutation_down = pshape_0->multirow_tr_permutation_down;
        pshape->multirow_tr_shape_up = pshape_0->multirow_tr_shape_up;
        pshape->multirow_tr_shape_down = pshape_0->multirow_tr_shape_down;
        pshape->width = pshape_0->width;
        pshape->height = 1;
        std::cout << "initial pshape width: " << pshape->multirow_tr_permutation_down[0].size() << std::endl;
        for (int i = 1; i < pshape_vec.size(); i++) {
            Pshape *pshape_connected = pshape_vec[i];

            // check merge_enable
            Transistor *pshape_tr_right_up = pshape->multirow_tr_permutation_up[0][pshape->multirow_tr_permutation_up.size() - 1];
            Transistor *pshape_tr_right_down = pshape->multirow_tr_permutation_down[0][pshape->multirow_tr_permutation_down.size() - 1];
            Transistor *pshape_tr_left_up = pshape->multirow_tr_permutation_up[0][0];
            Transistor *pshape_tr_left_down = pshape->multirow_tr_permutation_down[0][0];
            Transistor *pshape_connected_tr_right_up = pshape_connected->multirow_tr_permutation_up[0][pshape_connected->multirow_tr_permutation_up.size() - 1];
            Transistor *pshape_connected_tr_right_down =
                pshape_connected->multirow_tr_permutation_down[0][pshape_connected->multirow_tr_permutation_down.size() - 1];
            Transistor *pshape_connected_tr_left_up = pshape_connected->multirow_tr_permutation_up[0][0];
            Transistor *pshape_connected_tr_left_down = pshape_connected->multirow_tr_permutation_down[0][0];

            int pshape_shape_id_right_up = pshape->multirow_tr_shape_up[0][pshape->multirow_tr_shape_up.size()];
            int pshape_shape_id_right_down = pshape->multirow_tr_shape_down[0][pshape->multirow_tr_shape_down.size()];
            int pshape_shape_id_left_up = pshape->multirow_tr_shape_up[0][0];
            int pshape_shape_id_left_down = pshape->multirow_tr_shape_down[0][0];
            int pshape_connected_shape_id_right_up = pshape_connected->multirow_tr_shape_up[0][pshape_connected->multirow_tr_shape_up.size()];
            int pshape_connected_shape_id_right_down = pshape_connected->multirow_tr_shape_down[0][pshape_connected->multirow_tr_shape_down.size()];
            int pshape_connected_shape_id_left_up = pshape_connected->multirow_tr_shape_up[0][0];
            int pshape_connected_shape_id_left_down = pshape_connected->multirow_tr_shape_down[0][0];

            Signal *pshape_right_up_sig = get_right_active(pshape_tr_right_up, pshape_shape_id_right_up);
            Signal *pshape_left_up_sig = get_left_active(pshape_tr_left_up, pshape_shape_id_left_up);
            Signal *pshape_right_down_sig = get_right_active(pshape_tr_right_down, pshape_shape_id_right_down);
            Signal *pshape_left_down_sig = get_left_active(pshape_tr_left_down, pshape_shape_id_left_down);
            Signal *pshape_connected_right_up_sig = get_right_active(pshape_connected_tr_right_up, pshape_connected_shape_id_right_up);
            Signal *pshape_connected_left_up_sig = get_left_active(pshape_connected_tr_left_up, pshape_connected_shape_id_left_up);
            Signal *pshape_connected_right_down_sig = get_right_active(pshape_connected_tr_right_down, pshape_connected_shape_id_right_down);
            Signal *pshape_connected_left_down_sig = get_left_active(pshape_connected_tr_left_down, pshape_connected_shape_id_left_down);

            bool merge_enable_right = (pshape_right_up_sig == pshape_connected_left_up_sig) && (pshape_right_down_sig == pshape_connected_left_down_sig);
            bool merge_enable_left = (pshape_left_up_sig == pshape_connected_right_up_sig) && (pshape_left_down_sig == pshape_connected_right_down_sig);

            if (merge_enable_right) {
                Pshape *pshape_merged = new Pshape();
                pshape_merged->multirow_tr_permutation_up = pshape->multirow_tr_permutation_up;
                pshape_merged->multirow_tr_permutation_down = pshape->multirow_tr_permutation_down;
                pshape_merged->multirow_tr_shape_up = pshape->multirow_tr_shape_up;
                pshape_merged->multirow_tr_shape_down = pshape->multirow_tr_shape_down;
                pshape_merged->multirow_tr_permutation_up[0].insert(pshape_merged->multirow_tr_permutation_up[0].end(),
                                                                    pshape_connected->multirow_tr_permutation_up[0].begin(),
                                                                    pshape_connected->multirow_tr_permutation_up[0].end());
                pshape_merged->multirow_tr_permutation_down[0].insert(pshape_merged->multirow_tr_permutation_down[0].end(),
                                                                      pshape_connected->multirow_tr_permutation_down[0].begin(),
                                                                      pshape_connected->multirow_tr_permutation_down[0].end());
                pshape_merged->multirow_tr_shape_up[0].insert(pshape_merged->multirow_tr_shape_up[0].end(), pshape_connected->multirow_tr_shape_up[0].begin(),
                                                              pshape_connected->multirow_tr_shape_up[0].end());
                pshape_merged->multirow_tr_shape_down[0].insert(pshape_merged->multirow_tr_shape_down[0].end(),
                                                                pshape_connected->multirow_tr_shape_down[0].begin(),
                                                                pshape_connected->multirow_tr_shape_down[0].end());
                pshape_merged->width = pshape->width + pshape_connected->width;
                pshape_merged->height = 1;
                pshape_merged->hsp = hsp(pshape_merged);
                pshape_merged->hcd = hcd(pshape_merged);
                pshape = pshape_merged;
                std::cout << "(diffusion sharing)pshape width: " << pshape->width << std::endl;
            } else {
                // merge right
                Pshape *pshape_merged1 = new Pshape();
                Pshape *pshape_merged2 = new Pshape();
                Pshape *pshape_connected_flipped = flipped(pshape_connected);
                pshape_merged1->multirow_tr_permutation_up = pshape->multirow_tr_permutation_up;
                pshape_merged1->multirow_tr_permutation_down = pshape->multirow_tr_permutation_down;
                pshape_merged1->multirow_tr_shape_up = pshape->multirow_tr_shape_up;
                pshape_merged1->multirow_tr_shape_down = pshape->multirow_tr_shape_down;
                pshape_merged1->multirow_tr_permutation_up[0].push_back(nullptr);
                pshape_merged1->multirow_tr_permutation_down[0].push_back(nullptr);
                pshape_merged1->multirow_tr_shape_up[0].push_back(2);
                pshape_merged1->multirow_tr_shape_down[0].push_back(2);
                pshape_merged1->multirow_tr_permutation_up[0].insert(pshape_merged1->multirow_tr_permutation_up[0].end(),
                                                                     pshape_connected->multirow_tr_permutation_up[0].begin(),
                                                                     pshape_connected->multirow_tr_permutation_up[0].end());
                pshape_merged1->multirow_tr_permutation_down[0].insert(pshape_merged1->multirow_tr_permutation_down[0].end(),
                                                                       pshape_connected->multirow_tr_permutation_down[0].begin(),
                                                                       pshape_connected->multirow_tr_permutation_down[0].end());
                pshape_merged1->multirow_tr_shape_up[0].insert(pshape_merged1->multirow_tr_shape_up[0].end(), pshape_connected->multirow_tr_shape_up[0].begin(),
                                                               pshape_connected->multirow_tr_shape_up[0].end());
                pshape_merged1->multirow_tr_shape_down[0].insert(pshape_merged1->multirow_tr_shape_down[0].end(),
                                                                 pshape_connected->multirow_tr_shape_down[0].begin(),
                                                                 pshape_connected->multirow_tr_shape_down[0].end());
                pshape_merged1->width = pshape->width + pshape_connected->width + 1;
                pshape_merged1->height = 1;

                pshape_merged2->multirow_tr_permutation_up = pshape->multirow_tr_permutation_up;
                pshape_merged2->multirow_tr_permutation_down = pshape->multirow_tr_permutation_down;
                pshape_merged2->multirow_tr_shape_up = pshape->multirow_tr_shape_up;
                pshape_merged2->multirow_tr_shape_down = pshape->multirow_tr_shape_down;
                pshape_merged2->multirow_tr_permutation_up[0].push_back(nullptr);
                pshape_merged2->multirow_tr_permutation_down[0].push_back(nullptr);
                pshape_merged2->multirow_tr_shape_up[0].push_back(2);
                pshape_merged2->multirow_tr_shape_down[0].push_back(2);
                pshape_merged2->multirow_tr_permutation_up[0].insert(pshape_merged2->multirow_tr_permutation_up[0].end(),
                                                                     pshape_connected_flipped->multirow_tr_permutation_up[0].begin(),
                                                                     pshape_connected_flipped->multirow_tr_permutation_up[0].end());
                pshape_merged2->multirow_tr_permutation_down[0].insert(pshape_merged2->multirow_tr_permutation_down[0].end(),
                                                                       pshape_connected_flipped->multirow_tr_permutation_down[0].begin(),
                                                                       pshape_connected_flipped->multirow_tr_permutation_down[0].end());
                pshape_merged2->multirow_tr_shape_up[0].insert(pshape_merged2->multirow_tr_shape_up[0].end(),
                                                               pshape_connected_flipped->multirow_tr_shape_up[0].begin(),
                                                               pshape_connected_flipped->multirow_tr_shape_up[0].end());
                pshape_merged2->multirow_tr_shape_down[0].insert(pshape_merged2->multirow_tr_shape_down[0].end(),
                                                                 pshape_connected_flipped->multirow_tr_shape_down[0].begin(),
                                                                 pshape_connected_flipped->multirow_tr_shape_down[0].end());
                pshape_merged2->width = pshape->width + pshape_connected->width + 1;
                pshape_merged2->height = 1;

                pshape_merged1->hsp = hsp(pshape_merged1);
                pshape_merged1->hcd = hcd(pshape_merged1);
                pshape_merged2->hsp = hsp(pshape_merged2);
                pshape_merged2->hcd = hcd(pshape_merged2);
                std::cout << "pshape_merged1->hsp: " << pshape_merged1->hsp << " pshape_merged2->hsp: " << pshape_merged2->hsp << std::endl;

                if (pshape_merged1->hsp < pshape_merged2->hsp) {
                    pshape = pshape_merged1;
                } else {
                    pshape = pshape_merged2;
                }

                std::cout << "pshape width: " << pshape->width << std::endl;
            }
        }
        results.push_back(pshape);
    }
    std::sort(results.begin(), results.end(), comparePshapeHSP);
    return results;
}

void calculate_pgr_blocked(Pshape *pshape) {
    pshape->multirow_pgr_blocked_vdd.assign(pshape->multirow_tr_permutation_up[0].size() * 2 + 1, 0);
    pshape->multirow_pgr_blocked_vss.assign(pshape->multirow_tr_permutation_up[0].size() * 2 + 1, 0);
    for (int k = 0; k < pshape->multirow_tr_permutation_down[0].size(); k++) {
        Transistor *tr_p = pshape->multirow_tr_permutation_down[0][k];
        Transistor *tr_n = pshape->multirow_tr_permutation_up[0][k];
        if (tr_p == nullptr) continue;
        if (tr_p->drain->name == "VDD") {
            if (pshape->multirow_tr_shape_down[0][k] == 0) {
                pshape->multirow_pgr_blocked_vdd[std::max(0, 2 * k - 1)] = 1;
                pshape->multirow_pgr_blocked_vdd[2 * k] = 1;
                pshape->multirow_pgr_blocked_vdd[2 * k + 1] = 1;
            } else {
                pshape->multirow_pgr_blocked_vdd[2 * k + 1] = 1;
                pshape->multirow_pgr_blocked_vdd[2 * k + 2] = 1;
                pshape->multirow_pgr_blocked_vdd[std::min(static_cast<int>(pshape->multirow_pgr_blocked_vdd.size()), 2 * k + 3)] = 1;
            }
        } else if (tr_p->source->name == "VDD") {
            if (pshape->multirow_tr_shape_down[0][k] == 1) {
                pshape->multirow_pgr_blocked_vdd[std::max(0, 2 * k - 1)] = 1;
                pshape->multirow_pgr_blocked_vdd[2 * k] = 1;
                pshape->multirow_pgr_blocked_vdd[2 * k + 1] = 1;
            } else {
                pshape->multirow_pgr_blocked_vdd[2 * k + 1] = 1;
                pshape->multirow_pgr_blocked_vdd[2 * k + 2] = 1;
                pshape->multirow_pgr_blocked_vdd[std::min(static_cast<int>(pshape->multirow_pgr_blocked_vdd.size()), 2 * k + 3)] = 1;
            }
        }
        if (tr_n == nullptr) continue;
        if (tr_n->drain->name == "VSS") {
            if (pshape->multirow_tr_shape_down[0][k] == 0) {
                pshape->multirow_pgr_blocked_vss[std::max(0, 2 * k - 1)] = 1;
                pshape->multirow_pgr_blocked_vss[2 * k] = 1;
                pshape->multirow_pgr_blocked_vss[2 * k + 1] = 1;
            } else {
                pshape->multirow_pgr_blocked_vss[2 * k + 1] = 1;
                pshape->multirow_pgr_blocked_vss[2 * k + 2] = 1;
                pshape->multirow_pgr_blocked_vss[std::min(static_cast<int>(pshape->multirow_pgr_blocked_vss.size()), 2 * k + 3)] = 1;
            }
        } else if (tr_n->source->name == "VSS") {
            if (pshape->multirow_tr_shape_down[0][k] == 1) {
                pshape->multirow_pgr_blocked_vss[std::max(0, 2 * k - 1)] = 1;
                pshape->multirow_pgr_blocked_vss[2 * k] = 1;
                pshape->multirow_pgr_blocked_vss[2 * k + 1] = 1;
            } else {
                pshape->multirow_pgr_blocked_vss[2 * k + 1] = 1;
                pshape->multirow_pgr_blocked_vss[2 * k + 2] = 1;
                pshape->multirow_pgr_blocked_vss[std::min(static_cast<int>(pshape->multirow_pgr_blocked_vss.size()), 2 * k + 3)] = 1;
            }
        }
    }
}

void generate_plmt(std::vector<Pshape *> pshape_vec) {
    // 指定資料夾名稱
    std::string folderName = "results";

    if (fs::exists(folderName)) {
        // 刪除資料夾及其內容
        fs::remove_all(folderName);
        std::cout << "資料夾 " << folderName << " 已刪除。" << std::endl;
    } else {
        std::cout << "資料夾 " << folderName << " 不存在。" << std::endl;
    }

    // 創建資料夾
    if (createFolder(folderName) == 0) {
        std::cout << "資料夾 " << folderName << " 已成功創建！" << std::endl;
    } else {
        std::cerr << "無法創建資料夾 " << folderName << "，可能已存在。" << std::endl;
    }
    for (int i = 0; i < pshape_vec.size(); i++) {
        // 將輸出寫入該資料夾中的不同檔案
        Pshape *pshape = pshape_vec[i];
        // 構造檔案路徑
        std::string fileName = folderName + "/output_" + std::to_string(i) + ".plmt";

        // 打開文件輸出流
        std::ofstream outputFile(fileName);
        if (!outputFile) {
            std::cerr << "無法創建檔案 " << fileName << std::endl;
            continue;
        }

        // 寫入內容
        outputFile << "<NAME> " << i << std::endl;
        outputFile << "<OUTPUT> ";
        for (Signal *sig : outputs) {
            for (Transistor *tr : pshape->multirow_tr_permutation_down[0]) {
                if (tr == nullptr) continue;
                if (tr->drain == sig || tr->source == sig) {
                    outputFile << sig->name << " ";
                    break;
                }
            }
        }
        outputFile << std::endl;
        outputFile << "<POWER> VDD" << std::endl;
        outputFile << "<GROUND> VSS" << std::endl;
        outputFile << "<INPUT> ";
        for (Signal *sig : inputs) {
            for (Transistor *tr : pshape->multirow_tr_permutation_down[0]) {
                if (tr == nullptr) continue;
                if (tr->gate == sig) {
                    outputFile << sig->name << " ";
                    break;
                }
            }
        }
        outputFile << std::endl;
        outputFile << "<PMOS> " << pshape->width << " ( ";
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                Transistor *tr_p = pshape->multirow_tr_permutation_down[i][j];
                switch (pshape->multirow_tr_shape_down[i][j]) {
                    case 0:
                        outputFile << tr_p->width / tr_p->num_finger << " ";
                        break;
                    case 1:
                        outputFile << tr_p->width / tr_p->num_finger << " ";
                        break;
                    case 2:
                        outputFile << "81 ";
                        break;
                }
            }
            outputFile << ")" << std::endl;
        }
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                Transistor *tr_p = pshape->multirow_tr_permutation_down[i][j];
                switch (pshape->multirow_tr_shape_down[i][j]) {
                    case 0:
                        outputFile << tr_p->drain->name << " <" << tr_p->gate->name << "> " << tr_p->source->name << " ";
                        break;
                    case 1:
                        outputFile << tr_p->source->name << " <" << tr_p->gate->name << "> " << tr_p->drain->name << " ";
                        break;
                    case 2:
                        Transistor *tr_p_pre = pshape->multirow_tr_permutation_down[i][j - 1];
                        Transistor *tr_p_post = pshape->multirow_tr_permutation_down[i][j + 1];
                        switch (pshape->multirow_tr_shape_down[i][j - 1]) {
                            case 0:
                                outputFile << tr_p_pre->source->name << " ";
                                break;
                            case 1:
                                outputFile << tr_p_pre->drain->name << " ";
                                break;
                        }
                        outputFile << "<VDD> ";
                        switch (pshape->multirow_tr_shape_down[i][j + 1]) {
                            case 0:
                                outputFile << tr_p_post->drain->name << " ";
                                break;
                            case 1:
                                outputFile << tr_p_post->source->name << " ";
                                break;
                        }
                        break;
                }
            }
            outputFile << std::endl;
        }
        outputFile << std::endl;
        outputFile << "<NMOS> " << pshape->width << " ( ";
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                Transistor *tr_n = pshape->multirow_tr_permutation_up[i][j];
                switch (pshape->multirow_tr_shape_up[i][j]) {
                    case 0:
                        outputFile << tr_n->width / tr_n->num_finger << " ";
                        break;
                    case 1:
                        outputFile << tr_n->width / tr_n->num_finger << " ";
                        break;
                    case 2:
                        outputFile << "81";
                        break;
                }
            }
            outputFile << ")" << std::endl;
        }
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                Transistor *tr_n = pshape->multirow_tr_permutation_up[i][j];
                switch (pshape->multirow_tr_shape_up[i][j]) {
                    case 0:
                        outputFile << tr_n->drain->name << " <" << tr_n->gate->name << "> " << tr_n->source->name << " ";
                        break;
                    case 1:
                        outputFile << tr_n->source->name << " <" << tr_n->gate->name << "> " << tr_n->drain->name << " ";
                        break;
                    case 2:
                        Transistor *tr_n_pre = pshape->multirow_tr_permutation_up[i][j - 1];
                        Transistor *tr_n_post = pshape->multirow_tr_permutation_up[i][j + 1];
                        switch (pshape->multirow_tr_shape_up[i][j - 1]) {
                            case 0:
                                outputFile << tr_n_pre->source->name << " ";
                                break;
                            case 1:
                                outputFile << tr_n_pre->drain->name << " ";
                                break;
                        }
                        outputFile << "<VSS> ";
                        switch (pshape->multirow_tr_shape_up[i][j + 1]) {
                            case 0:
                                outputFile << tr_n_post->source->name << " ";
                                break;
                            case 1:
                                outputFile << tr_n_post->drain->name << " ";
                                break;
                        }
                        break;
                }
            }
            outputFile << std::endl;
        }
        outputFile.close();

        std::cout << "內容已寫入檔案 " << fileName << std::endl;
    }
}

Pshape *flipped(Pshape *pshape) {
    Pshape *result = new Pshape();
    result->multirow_tr_permutation_up = pshape->multirow_tr_permutation_up;
    result->multirow_tr_permutation_down = pshape->multirow_tr_permutation_down;
    result->multirow_tr_shape_up = pshape->multirow_tr_shape_up;
    result->multirow_tr_shape_down = pshape->multirow_tr_shape_down;
    for (int i = 0; i < pshape->multirow_tr_permutation_up[0].size(); i++) {
        result->multirow_tr_permutation_up[0][pshape->multirow_tr_permutation_up[0].size() - 1 - i] = pshape->multirow_tr_permutation_up[0][i];
        result->multirow_tr_permutation_down[0][pshape->multirow_tr_permutation_down[0].size() - 1 - i] = pshape->multirow_tr_permutation_down[0][i];
        result->multirow_tr_shape_up[0][pshape->multirow_tr_shape_up[0].size() - 1 - i] = (pshape->multirow_tr_shape_up[0][i] == 1)   ? 0
                                                                                          : (pshape->multirow_tr_shape_up[0][i] == 0) ? 1
                                                                                                                                      : 2;
        result->multirow_tr_shape_down[0][pshape->multirow_tr_shape_down[0].size() - 1 - i] = (pshape->multirow_tr_shape_down[0][i] == 1)   ? 0
                                                                                              : (pshape->multirow_tr_shape_down[0][i] == 0) ? 1
                                                                                                                                            : 2;
    }
    return result;
}

int hsp(Pshape *pshape) {  // Horizontal Span of all Routing Signals
    std::vector<Signal *> signal_vec_p;
    std::vector<Signal *> signal_vec_n;
    signal_vec_p.assign(pshape->multirow_tr_permutation_down[0].size() * 2 + 1, nullptr);
    signal_vec_n.assign(pshape->multirow_tr_permutation_up[0].size() * 2 + 1, nullptr);
    for (int i = 0; i < pshape->multirow_tr_permutation_down[0].size(); i++) {
        Transistor *tr_p = pshape->multirow_tr_permutation_down[0][i];
        Transistor *tr_n = pshape->multirow_tr_permutation_up[0][i];
        if (tr_p == nullptr) continue;
        if (tr_n == nullptr) continue;
        signal_vec_p[i * 2] = get_left_active(tr_p, pshape->multirow_tr_shape_down[0][i]);
        signal_vec_n[i * 2] = get_left_active(tr_n, pshape->multirow_tr_shape_up[0][i]);
        signal_vec_p[i * 2 + 1] = tr_p->gate;
        signal_vec_n[i * 2 + 1] = tr_n->gate;
        signal_vec_p[i * 2 + 2] = get_right_active(tr_p, pshape->multirow_tr_shape_down[0][i]);
        signal_vec_n[i * 2 + 2] = get_right_active(tr_n, pshape->multirow_tr_shape_up[0][i]);
    }
    std::set<Signal *> signal_set_p(signal_vec_p.begin(), signal_vec_p.end());
    std::set<Signal *> signal_set_n(signal_vec_n.begin(), signal_vec_n.end());

    int result = 0;
    for (Signal *sig : signal_set_p) {
        if (sig == nullptr) continue;
        sig->start = -1;
        sig->end = -1;
        if (sig->name == "VDD" || sig->name == "VSS") continue;
        for (int i = 0; i < signal_vec_p.size(); i++) {
            if (signal_vec_p[i] == nullptr) continue;
            if (signal_vec_p[i] == sig) {
                if (sig->start == -1) {
                    sig->start = i;
                }
                sig->end = i;
            }
        }
        result = result + sig->end - sig->start;
    }
    for (Signal *sig : signal_set_n) {
        if (sig == nullptr) continue;
        if (sig->name == "VDD" || sig->name == "VSS") continue;
        sig->start = -1;
        sig->end = -1;
        for (int i = 0; i < signal_vec_n.size(); i++) {
            if (signal_vec_n[i] == nullptr) continue;
            if (signal_vec_n[i] == sig) {
                if (sig->start == -1) {
                    sig->start = i;
                }
                sig->end = i;
            }
        }
        result = result + sig->end - sig->start;
    }
    return result;
}

int hcd(Pshape *pshape) {
    int result = 0;
    std::vector<int> density_vec;
    density_vec.assign(pshape->multirow_tr_permutation_up[0].size() * 2 + 1, 0);

    std::vector<Signal *> signal_vec_p;
    std::vector<Signal *> signal_vec_n;
    signal_vec_p.assign(pshape->multirow_tr_permutation_down[0].size() * 2 + 1, nullptr);
    signal_vec_n.assign(pshape->multirow_tr_permutation_up[0].size() * 2 + 1, nullptr);
    for (int i = 0; i < pshape->multirow_tr_permutation_down[0].size(); i++) {
        Transistor *tr_p = pshape->multirow_tr_permutation_down[0][i];
        Transistor *tr_n = pshape->multirow_tr_permutation_up[0][i];
        if (tr_p == nullptr) continue;
        if (tr_n == nullptr) continue;
        signal_vec_p[i * 2] = get_left_active(tr_p, pshape->multirow_tr_shape_down[0][i]);
        signal_vec_n[i * 2] = get_left_active(tr_n, pshape->multirow_tr_shape_up[0][i]);
        signal_vec_p[i * 2 + 1] = tr_p->gate;
        signal_vec_n[i * 2 + 1] = tr_n->gate;
        signal_vec_p[i * 2 + 2] = get_right_active(tr_p, pshape->multirow_tr_shape_down[0][i]);
        signal_vec_n[i * 2 + 2] = get_right_active(tr_n, pshape->multirow_tr_shape_up[0][i]);
    }
    std::set<Signal *> signal_set_p(signal_vec_p.begin(), signal_vec_p.end());
    std::set<Signal *> signal_set_n(signal_vec_n.begin(), signal_vec_n.end());

    for (Signal *sig : signal_set_p) {
        if (sig == nullptr) continue;
        sig->start = -1;
        sig->end = -1;
        if (sig->name == "VDD" || sig->name == "VSS") continue;
        for (int i = 0; i < signal_vec_p.size(); i++) {
            if (signal_vec_p[i] == nullptr) continue;
            if (signal_vec_p[i] == sig) {
                if (sig->start == -1) {
                    sig->start = i;
                }
                sig->end = i;
            }
        }
    }
    for (Signal *sig : signal_set_n) {
        if (sig == nullptr) continue;
        if (sig->name == "VDD" || sig->name == "VSS") continue;
        sig->start = -1;
        sig->end = -1;
        for (int i = 0; i < signal_vec_n.size(); i++) {
            if (signal_vec_n[i] == nullptr) continue;
            if (signal_vec_n[i] == sig) {
                if (sig->start == -1) {
                    sig->start = i;
                }
                sig->end = i;
            }
        }
    }

    for (Signal *sig : signal_set_p) {
        if (sig == nullptr) continue;
        if (sig->name == "VDD" || sig->name == "VSS") continue;
        for (int i = sig->start; i <= sig->end; i++) {
            density_vec[i] = density_vec[i] + 1;
        }
    }
    for (Signal *sig : signal_set_n) {
        if (sig == nullptr) continue;
        if (sig->name == "VDD" || sig->name == "VSS") continue;
        for (int i = sig->start; i <= sig->end; i++) {
            // density_vec[i] = density_vec[i] + 1;
        }
    }
    for (int i = 0; i < density_vec.size(); i++) {
        result = std::max(density_vec[i], result);
    }

    return result;
}

std::vector<Pshape *> multirow_assignment(std::vector<Pshape *> pshape_vec) {
    std::vector<std::vector<Pshape *>> result;

    std::sort(pshape_vec.begin(), pshape_vec.end());

    do {
        result.push_back(pshape_vec);
    } while (std::next_permutation(pshape_vec.begin(), pshape_vec.end()));

    int min = std::numeric_limits<int>::max();
    std::vector<Pshape *> result_pshape;
    for (int i = 0; i < result.size(); i++) {
        int inter_row_signal_num = inter_row_signal_count(result[i]);
        if (inter_row_signal_num < min) {
            min = inter_row_signal_num;
            result_pshape = result[i];
        }
    }
    std::cout << "inter_row_signal_count: " << inter_row_signal_count(result_pshape) << std::endl;
    return result_pshape;
}

int inter_row_signal_count(std::vector<Pshape *> pshape_vec) {
    std::set<Signal *> signal_set;
    for (int i = 0; i < pshape_vec.size(); i++) {
        Pshape *pshape = pshape_vec[i];
        for (int j = 0; j < pshape->multirow_tr_permutation_down[0].size(); j++) {
            Transistor *tr_p = pshape->multirow_tr_permutation_down[0][j];
            Transistor *tr_n = pshape->multirow_tr_permutation_up[0][j];
            if (tr_p != nullptr) {
                signal_set.insert(tr_p->drain);
                signal_set.insert(tr_p->gate);
                signal_set.insert(tr_p->source);
            }
            if (tr_n != nullptr) {
                signal_set.insert(tr_n->drain);
                signal_set.insert(tr_n->gate);
                signal_set.insert(tr_n->source);
            }
        }
    }
    signal_set.erase(signals["VDD"]);
    signal_set.erase(signals["VSS"]);
    int result = 0;
    for (Signal *sig : signal_set) {
        int start = -1;
        int end = -1;
        for (int i = 0; i < pshape_vec.size(); i++) {
            Pshape *pshape = pshape_vec[i];
            for (int j = 0; j < pshape->multirow_tr_permutation_down[0].size(); j++) {
                Transistor *tr_p = pshape->multirow_tr_permutation_down[0][j];
                Transistor *tr_n = pshape->multirow_tr_permutation_up[0][j];
                if (tr_p != nullptr) {
                    if (sig == tr_p->drain || sig == tr_p->gate || sig == tr_p->source) {
                        if (start == -1) {
                            start = i;
                        }
                        end = i;
                    }
                }
                if (tr_n != nullptr) {
                    if (sig == tr_n->drain || sig == tr_n->gate || sig == tr_n->source) {
                        if (start == -1) {
                            start = i;
                        }
                        end = i;
                    }
                }
            }
        }
        result = result + end - start;
    }
    // std::cout << "inter-row #signal: " << result << std::endl;
    return result;
}

int max_cfet_width = 81.0;
int diffusion_break_constraint = 1;
int max_placement_size = 1024;
int max_allowable_cell_height = 1;
int num_nodes_parsed_to_gpu = 10000;
float expected_row_num = 1;
float relaxation_parameter = 0;
float aspect_ratio = 0.8;