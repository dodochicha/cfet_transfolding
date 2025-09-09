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
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
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
std::set<Signal *> outputs;
std::set<Signal *> inputs;
std::vector<Transistor *> trs;
std::vector<Transistor *> pmos;
std::vector<Transistor *> nmos;
std::vector<std::vector<Signal *>> via_preassignment;
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

bool compareRoutability(const Pshape *a, const Pshape *b) { return a->hsp + 100 * a->hcd < b->hsp + 100 * b->hcd; }
// bool compareRoutability(const Pshape *a, const Pshape *b) { return a->hpml < b->hpml; }

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
                        // pshape->multirow_signal_permutation_up[i][j * 2] = nullptr;
                        // pshape->multirow_signal_permutation_up[i][j * 2 + 1] = nullptr;
                        // pshape->multirow_signal_permutation_up[i][j * 2 + 2] = nullptr;
                        break;
                }
            }
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
                        // pshape->multirow_signal_permutation_down[i][j * 2] = nullptr;
                        // pshape->multirow_signal_permutation_down[i][j * 2 + 1] = nullptr;
                        // pshape->multirow_signal_permutation_down[i][j * 2 + 2] = nullptr;
                        break;
                }
            }
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

Pshape *abut_on_top(Pshape *p1, Pshape *p2) {
    Pshape *p = new Pshape();
    const int rows = p1->multirow_tr_permutation_up.size() + p2->multirow_tr_permutation_up.size();
    const int cols = std::max(p1->multirow_tr_permutation_up[0].size(), p2->multirow_tr_permutation_up[0].size());
    p->multirow_tr_permutation_up.resize(rows);
    p->multirow_tr_permutation_down.resize(rows);
    p->multirow_signal_permutation_up.resize(rows);
    p->multirow_signal_permutation_down.resize(rows);
    p->multirow_tr_shape_up.resize(rows);
    p->multirow_tr_shape_down.resize(rows);
    for (int r = 0; r < p->multirow_tr_permutation_up.size(); r++) {
        p->multirow_tr_permutation_up[r].assign(cols, nullptr);
        p->multirow_tr_permutation_down[r].assign(cols, nullptr);
        p->multirow_signal_permutation_up[r].assign(2 * cols + 1, nullptr);
        p->multirow_signal_permutation_down[r].assign(2 * cols + 1, nullptr);
        p->multirow_tr_shape_up[r].assign(cols, 2);
        p->multirow_tr_shape_down[r].assign(cols, 2);
    }
    for (int r = 0; r < p1->multirow_tr_permutation_up.size(); r++) {
        for (int c = 0; c < p1->multirow_tr_permutation_up[r].size(); c++) {
            p->multirow_tr_permutation_up[r][c] = p1->multirow_tr_permutation_up[r][c];
            p->multirow_tr_permutation_down[r][c] = p1->multirow_tr_permutation_down[r][c];
            p->multirow_tr_shape_up[r][c] = p1->multirow_tr_shape_up[r][c];
            p->multirow_tr_shape_down[r][c] = p1->multirow_tr_shape_down[r][c];
        }
        for (int c = 0; c < p1->multirow_signal_permutation_up[r].size(); c++) {
            p->multirow_signal_permutation_up[r][c] = p1->multirow_signal_permutation_up[r][c];
            p->multirow_signal_permutation_down[r][c] = p1->multirow_signal_permutation_down[r][c];
        }
    }
    for (int r = p1->multirow_tr_permutation_up.size(); r < rows; r++) {
        for (int c = 0; c < p2->multirow_tr_permutation_up[r - p1->multirow_tr_permutation_up.size()].size(); c++) {
            p->multirow_tr_permutation_up[r][c] = p2->multirow_tr_permutation_up[r - p1->multirow_tr_permutation_up.size()][c];
            p->multirow_tr_permutation_down[r][c] = p2->multirow_tr_permutation_down[r - p1->multirow_tr_permutation_up.size()][c];
            p->multirow_tr_shape_up[r][c] = p2->multirow_tr_shape_up[r - p1->multirow_tr_permutation_up.size()][c];
            p->multirow_tr_shape_down[r][c] = p2->multirow_tr_shape_down[r - p1->multirow_tr_permutation_up.size()][c];
        }
        for (int c = 0; c < p2->multirow_signal_permutation_up[r - p1->multirow_tr_permutation_up.size()].size(); c++) {
            p->multirow_signal_permutation_up[r][c] = p2->multirow_signal_permutation_up[r - p1->multirow_tr_permutation_up.size()][c];
            p->multirow_signal_permutation_down[r][c] = p2->multirow_signal_permutation_down[r - p1->multirow_tr_permutation_up.size()][c];
        }
    }
    return p;
}
Pshape *abut_on_bottom(Pshape *p1, Pshape *p2) {
    Pshape *p = new Pshape();
    const int rows = p1->multirow_tr_permutation_up.size() + p2->multirow_tr_permutation_up.size();
    const int cols = std::max(p1->multirow_tr_permutation_up[0].size(), p2->multirow_tr_permutation_up[0].size());
    p->multirow_tr_permutation_up.resize(rows);
    p->multirow_tr_permutation_down.resize(rows);
    p->multirow_signal_permutation_up.resize(rows);
    p->multirow_signal_permutation_down.resize(rows);
    p->multirow_tr_shape_up.resize(rows);
    p->multirow_tr_shape_down.resize(rows);
    for (int r = 0; r < p->multirow_tr_permutation_up.size(); r++) {
        p->multirow_tr_permutation_up[r].assign(cols, nullptr);
        p->multirow_tr_permutation_down[r].assign(cols, nullptr);
        p->multirow_signal_permutation_up[r].assign(2 * cols + 1, nullptr);
        p->multirow_signal_permutation_down[r].assign(2 * cols + 1, nullptr);
        p->multirow_tr_shape_up[r].assign(cols, 2);
        p->multirow_tr_shape_down[r].assign(cols, 2);
    }
    for (int r = 0; r < p2->multirow_tr_permutation_up.size(); r++) {
        for (int c = 0; c < p2->multirow_tr_permutation_up[r].size(); c++) {
            p->multirow_tr_permutation_up[r][c] = p2->multirow_tr_permutation_up[r][c];
            p->multirow_tr_permutation_down[r][c] = p2->multirow_tr_permutation_down[r][c];
            p->multirow_tr_shape_up[r][c] = p2->multirow_tr_shape_up[r][c];
            p->multirow_tr_shape_down[r][c] = p2->multirow_tr_shape_down[r][c];
        }
        for (int c = 0; c < p2->multirow_signal_permutation_up[r].size(); c++) {
            p->multirow_signal_permutation_up[r][c] = p2->multirow_signal_permutation_up[r][c];
            p->multirow_signal_permutation_down[r][c] = p2->multirow_signal_permutation_down[r][c];
        }
    }
    for (int r = p2->multirow_tr_permutation_up.size(); r < rows; r++) {
        for (int c = 0; c < p1->multirow_tr_permutation_up[r - p2->multirow_tr_permutation_up.size()].size(); c++) {
            p->multirow_tr_permutation_up[r][c] = p1->multirow_tr_permutation_up[r - p2->multirow_tr_permutation_up.size()][c];
            p->multirow_tr_permutation_down[r][c] = p1->multirow_tr_permutation_down[r - p2->multirow_tr_permutation_up.size()][c];
            p->multirow_tr_shape_up[r][c] = p1->multirow_tr_shape_up[r - p2->multirow_tr_permutation_up.size()][c];
            p->multirow_tr_shape_down[r][c] = p1->multirow_tr_shape_down[r - p2->multirow_tr_permutation_up.size()][c];
        }
        for (int c = 0; c < p1->multirow_signal_permutation_up[r - p2->multirow_tr_permutation_up.size()].size(); c++) {
            p->multirow_signal_permutation_up[r][c] = p1->multirow_signal_permutation_up[r - p2->multirow_tr_permutation_up.size()][c];
            p->multirow_signal_permutation_down[r][c] = p1->multirow_signal_permutation_down[r - p2->multirow_tr_permutation_up.size()][c];
        }
    }
    return p;
}
Pshape *horizontal_flip(Pshape *p) {
    Pshape *new_p = new Pshape();
    const int rows = p->multirow_tr_permutation_up.size();
    int max_cols = 0;
    for (int r = 0; r < rows; r++) {
        if (p->multirow_tr_permutation_up[r].size() > max_cols) {
            max_cols = p->multirow_tr_permutation_up[r].size();
        }
    }
    const int cols = max_cols;
    new_p->multirow_tr_permutation_up.resize(rows);
    new_p->multirow_tr_permutation_down.resize(rows);
    new_p->multirow_signal_permutation_up.resize(rows);
    new_p->multirow_signal_permutation_down.resize(rows);
    new_p->multirow_tr_shape_up.resize(rows);
    new_p->multirow_tr_shape_down.resize(rows);
    for (int r = 0; r < new_p->multirow_tr_permutation_up.size(); r++) {
        new_p->multirow_tr_permutation_up[r].assign(cols, nullptr);
        new_p->multirow_tr_permutation_down[r].assign(cols, nullptr);
        new_p->multirow_signal_permutation_up[r].assign(2 * cols + 1, nullptr);
        new_p->multirow_signal_permutation_down[r].assign(2 * cols + 1, nullptr);
        new_p->multirow_tr_shape_up[r].assign(cols, 2);
        new_p->multirow_tr_shape_down[r].assign(cols, 2);
    }
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < p->multirow_tr_permutation_up[r].size(); c++) {
            new_p->multirow_tr_permutation_up[r][cols - c - 1] = p->multirow_tr_permutation_up[r][c];
            new_p->multirow_tr_permutation_down[r][cols - c - 1] = p->multirow_tr_permutation_down[r][c];
            new_p->multirow_tr_shape_up[r][cols - c - 1] = (p->multirow_tr_shape_up[r][c] == 0) ? 1 : (p->multirow_tr_shape_up[r][c] == 1) ? 0 : 2;
            new_p->multirow_tr_shape_down[r][cols - c - 1] = (p->multirow_tr_shape_down[r][c] == 0) ? 1 : (p->multirow_tr_shape_down[r][c] == 1) ? 0 : 2;
        }
        for (int c = 0; c < p->multirow_signal_permutation_up[r].size(); c++) {
            new_p->multirow_signal_permutation_up[r][2 * cols - c] = p->multirow_signal_permutation_up[r][c];
            new_p->multirow_signal_permutation_down[r][2 * cols - c] = p->multirow_signal_permutation_down[r][c];
        }
    }
    return new_p;
}

Pshape *placement_abutting(std::vector<Pshape *> pshape_vec) {
    std::set<Pshape *> pshape_set;
    for (int i = 1; i < pshape_vec.size(); i++) {
        pshape_set.insert(pshape_vec[i]);
    }
    Pshape *result = pshape_vec[0]->copy();
    while (pshape_set.size() != 0) {
        std::cout << "pshape_set.size(): " << pshape_set.size() << std::endl;
        Pshape *next_p;
        int min_hpml = std::numeric_limits<double>::infinity();
        bool flip = false;
        bool top = false;
        for (auto p : pshape_set) {
            Pshape *p_flip = horizontal_flip(p);
            Pshape *pt = abut_on_top(result, p);
            Pshape *ptf = abut_on_top(result, p_flip);
            Pshape *pb = abut_on_bottom(result, p);
            Pshape *pbf = abut_on_bottom(result, p_flip);
            if (pt->calculate_hpml() < min_hpml) {
                min_hpml = pt->calculate_hpml();
                next_p = p;
                flip = false;
                top = true;
            }
            if (ptf->calculate_hpml() < min_hpml) {
                min_hpml = ptf->calculate_hpml();
                next_p = p;
                flip = true;
                top = true;
            }
            if (pb->calculate_hpml() < min_hpml) {
                min_hpml = pt->calculate_hpml();
                next_p = p;
                flip = false;
                top = false;
            }
            if (pbf->calculate_hpml() < min_hpml) {
                min_hpml = pt->calculate_hpml();
                next_p = p;
                flip = true;
                top = false;
            }
        }
        pshape_set.erase(next_p);
        if (flip) {
            next_p = horizontal_flip(next_p);
        }
        if (top) {
            result = abut_on_top(result, next_p);
        } else {
            result = abut_on_bottom(result, next_p);
        }
        // result->print_pshape();
    }
    return result;
}

Pshape *placement_merging(std::vector<Pshape *> pshape_vec) {
    std::cout << "hello placement_merging" << std::endl;
    const int N = (int)pshape_vec.size();
    if (N == 0) return nullptr;

    int max_cols = 0;
    for (int i = 0; i < pshape_vec.size(); i++) {
        if (pshape_vec[i]->multirow_tr_permutation_up[0].size() > max_cols) {
            max_cols = pshape_vec[i]->multirow_tr_permutation_up[0].size();
        }
    }

    // 想固定 4x2 就打開下面兩行（N 必須為 8）; 否則預設 1 欄直排
    int rows = N, cols = 1;
    if (N == 8) {
        rows = 4;
        cols = 2;
    }

    // 狀態
    std::vector<int> pos2comp(N, -1);       // 位置 -> 元件 id
    std::vector<bool> flipAtPos(N, false);  // 位置 -> flip 狀態
    double bestScore = std::numeric_limits<double>::infinity();
    std::vector<int> bestPos2Comp(N, -1);
    std::vector<bool> bestFlipAtPos(N, false);

    // 位置索引 -> (r,c)
    auto pos_rc = [&](int idx) -> std::pair<int, int> { return std::make_pair(idx / cols, idx % cols); };

    auto score_partial = [&](int placed) -> int {
        if (placed <= 0) return 0;
        Pshape *mg_pshape = nullptr;
        for (int i = 0; i < placed; i++) {
            if (flipAtPos[i]) {
                Pshape *fp = horizontal_flip(pshape_vec[pos2comp[i]]);
                Pshape *new_p = pshape_insert(mg_pshape, fp, i / cols, (i % cols) * (max_cols + 1));
                delete mg_pshape;
                mg_pshape = new_p;
                delete fp;
            } else {
                Pshape *new_p = pshape_insert(mg_pshape, pshape_vec[pos2comp[i]], i / cols, (i % cols) * (max_cols + 1));
                delete mg_pshape;
                mg_pshape = new_p;
            }
        }
        int hpml = mg_pshape->calculate_hpml();
        delete mg_pshape;
        return hpml;
    };

    // 完整解分數
    auto score_full = [&]() -> double {
        Pshape *mg_pshape = nullptr;
        for (int i = 0; i < N; i++) {
            if (flipAtPos[i]) {
                Pshape *fp = horizontal_flip(pshape_vec[pos2comp[i]]);
                Pshape *new_p = pshape_insert(mg_pshape, fp, i / cols, (i % cols) * (max_cols + 1));
                delete mg_pshape;
                mg_pshape = new_p;
                delete fp;
            } else {
                Pshape *new_p = pshape_insert(mg_pshape, pshape_vec[pos2comp[i]], i / cols, (i % cols) * (max_cols + 1));
                delete mg_pshape;
                mg_pshape = new_p;
            }
        }
        int hpml = mg_pshape->calculate_hpml();
        // mg_pshape->print_pshape();
        delete mg_pshape;
        return hpml;
    };

    // （可選）先用簡單貪心產生上界，讓剪枝更積極；此處略
    // bestScore = initial_upper_bound(...);

    // DFS（auto + Y-combinator）
    auto dfs = [&](auto &&self, int posIdx, uint64_t usedMask) -> void {
        if (posIdx == N) {
            double s = score_full();
            for (int i = 0; i < pos2comp.size(); i++) {
                std::cout << pos2comp[i] << " ";
            }
            for (int i = 0; i < pos2comp.size(); i++) {
                std::cout << flipAtPos[i] << " ";
            }
            std::cout << std::endl;
            std::cout << "score: " << s << std::endl;
            if (s < bestScore) {
                bestScore = s;
                bestPos2Comp = pos2comp;
                bestFlipAtPos = flipAtPos;
            }
            return;
        }

        // 下界剪枝
        if (score_partial(posIdx) >= bestScore) return;

        // 對稱性消除（建議保留）：固定第一格放 comp 0 且不翻轉
        if (posIdx == 0) {
            int comp = 0;
            pos2comp[posIdx] = comp;
            flipAtPos[posIdx] = false;
            self(self, posIdx + 1, usedMask | (1ull << comp));
            pos2comp[posIdx] = -1;
            return;
        }

        // 枚舉所有未使用元件 + 該位置的 flip / not flip
        for (int comp = 0; comp < N; ++comp) {
            if (usedMask & (1ull << comp)) continue;

            pos2comp[posIdx] = comp;

            // not flip
            flipAtPos[posIdx] = false;
            if (score_partial(posIdx + 1) < bestScore) self(self, posIdx + 1, usedMask | (1ull << comp));

            // flip
            flipAtPos[posIdx] = true;
            if (score_partial(posIdx + 1) < bestScore) self(self, posIdx + 1, usedMask | (1ull << comp));

            pos2comp[posIdx] = -1;  // 回溯
        }
    };
    // 啟動搜尋
    dfs(dfs, 0, 0ull);

    // ===== 把最佳解組成一個新的 Pshape* （依你的 API 改）=====
    auto build_merged_pshape = [&](const std::vector<Pshape *> &src, const std::vector<int> &bestP2C, const std::vector<bool> &bestFlip, int R,
                                   int C) -> Pshape * {
        // TODO: 依你的 Pshape 介面實作：
        //  1) 對 bestP2C[k] 取出 src 裡的元件
        //  2) 若 bestFlip[k] 為真，做 flip（水平/垂直視你的定義）
        //  3) 依 (r,c) = (k/C, k%C) 計算平移量，擺到對應格子
        //  4) 逐一 merge 成單一 Pshape
        //  5) 回傳新物件指標（或智能指標）
        Pshape *mg_pshape = nullptr;
        for (int i = 0; i < N; i++) {
            if (bestFlip[i]) {
                mg_pshape = pshape_insert(mg_pshape, horizontal_flip(pshape_vec[bestP2C[i]]), i / cols, (i % cols) * (max_cols + 1));
            } else {
                mg_pshape = pshape_insert(mg_pshape, pshape_vec[bestP2C[i]], i / cols, (i % cols) * (max_cols + 1));
            }
        }
        return mg_pshape;
    };

    return build_merged_pshape(pshape_vec, bestPos2Comp, bestFlipAtPos, rows, cols);
}

Pshape *pshape_insert(Pshape *ori_pshape, Pshape *ins_pshape, int row, int col) {
    assert(col >= 0);
    assert(row >= 0);
    // std::cout << "row/col: " << row << "/" << col << std::endl;
    if (ori_pshape == nullptr) {
        ori_pshape = new Pshape();
        ori_pshape->multirow_tr_permutation_up.resize(1);
        ori_pshape->multirow_tr_permutation_up[0].resize(1);
        ori_pshape->multirow_tr_permutation_down.resize(1);
        ori_pshape->multirow_tr_permutation_down[0].resize(1);
        ori_pshape->multirow_tr_shape_up.resize(1);
        ori_pshape->multirow_tr_shape_up[0].resize(1);
        ori_pshape->multirow_tr_shape_down.resize(1);
        ori_pshape->multirow_tr_shape_down[0].resize(1);
    }

    Pshape *result = ori_pshape->copy();
    const int ori_row = ori_pshape->multirow_tr_permutation_up.size();
    const int ori_col = ori_pshape->multirow_tr_permutation_up[0].size();
    const int ins_row = ins_pshape->multirow_tr_permutation_up.size();
    const int ins_col = ins_pshape->multirow_tr_permutation_up[0].size();
    if (row + ins_row > ori_row) {
        for (int i = 0; i < row + ins_row - ori_row; i++) {
            result->multirow_tr_permutation_up.push_back(std::vector<Transistor *>());
            result->multirow_tr_permutation_down.push_back(std::vector<Transistor *>());
            result->multirow_tr_shape_up.push_back(std::vector<int>());
            result->multirow_tr_shape_down.push_back(std::vector<int>());
        }
    }
    for (int r = 0; r < result->multirow_tr_permutation_up.size(); r++) {
        while (result->multirow_tr_permutation_up[r].size() < col + ins_col) {
            result->multirow_tr_permutation_up[r].push_back(nullptr);
        }
        while (result->multirow_tr_permutation_down[r].size() < col + ins_col) {
            result->multirow_tr_permutation_down[r].push_back(nullptr);
        }
        while (result->multirow_tr_shape_up[r].size() < col + ins_col) {
            result->multirow_tr_shape_up[r].push_back(2);
        }
        while (result->multirow_tr_shape_down[r].size() < col + ins_col) {
            result->multirow_tr_shape_down[r].push_back(2);
        }
    }

    for (int r = 0; r < ins_row; r++) {
        for (int c = 0; c < ins_col; c++) {
            result->multirow_tr_permutation_up[row + r][col + c] = ins_pshape->multirow_tr_permutation_up[r][c];
            result->multirow_tr_permutation_down[row + r][col + c] = ins_pshape->multirow_tr_permutation_down[r][c];
            result->multirow_tr_shape_up[row + r][col + c] = ins_pshape->multirow_tr_shape_up[r][c];
            result->multirow_tr_shape_down[row + r][col + c] = ins_pshape->multirow_tr_shape_down[r][c];
        }
    }

    result->allign();

    return result;
}

int max_cfet_width = 81.0;
int diffusion_break_constraint = 1;
int max_placement_size = 16384;
int max_allowable_cell_height = 1;
int num_nodes_parsed_to_gpu = 10000;
float expected_row_num = 1;
float relaxation_parameter = 0;
float aspect_ratio = 12;