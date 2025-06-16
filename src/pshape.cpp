#include <omp.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "cfet.h"
#include "z3++.h"

namespace fs = std::filesystem;

// 如果不存在就創建資料夾
inline bool createFolder(const std::string &name) { return fs::create_directory(name); }

std::pair<Signal *, Signal *> Pshape::get_most_left_sig(int row) {
    for (int i = multirow_signal_permutation_up[row].size() - 1; i >= 0; i--) {
        Signal *sig_up = multirow_signal_permutation_up[row][i];
        Signal *sig_down = multirow_signal_permutation_down[row][i];
        if (sig_up) {
            return std::make_pair(sig_up, sig_down);
        }
    }
}

void Pshape::allign() {
    std::cout << "hello allign" << std::endl;
    std::vector<std::vector<Transistor *>> tr_up;
    std::vector<std::vector<Transistor *>> tr_down;
    std::vector<std::vector<int>> tr_shape_up;
    std::vector<std::vector<int>> tr_shape_down;
    std::vector<std::vector<Signal *>> sig_up;
    std::vector<std::vector<Signal *>> sig_down;
    const int row = multirow_tr_permutation_up.size();
    tr_up.resize(row);
    tr_down.resize(row);
    tr_shape_up.resize(row);
    tr_shape_down.resize(row);
    sig_up.resize(row);
    sig_down.resize(row);

    // for (int i = 0; i < row; i++) {
    //     for (int j = 0; j < multirow_tr_permutation_up[i].size(); j++) {
    //         Transistor *tr = multirow_tr_permutation_up[i][j];
    //         if (tr) {
    //             std::cout << tr->name << " ";
    //         } else {
    //             std::cout << "null ";
    //         }
    //     }
    //     std::cout << std::endl;
    // }

    for (int i = 0; i < row; i++) {
        Signal *pre_sig_up = nullptr;
        Signal *pre_sig_down = nullptr;
        bool first_insert = false;
        for (int j = 0; j < multirow_tr_permutation_up[i].size(); j++) {
            Transistor *current_tr_up = multirow_tr_permutation_up[i][j];
            Transistor *current_tr_down = multirow_tr_permutation_down[i][j];
            // if (current_tr_up) {
            //     std::cout << current_tr_up->name << " ";
            // } else {
            //     std::cout << "null ";
            // }
            int current_shape_up = multirow_tr_shape_up[i][j];
            int current_shape_down = multirow_tr_shape_down[i][j];
            Signal *current_sig_up = get_left_active(current_tr_up, current_shape_up);
            Signal *current_sig_down = get_left_active(current_tr_down, current_shape_down);
            if ((pre_sig_up == nullptr || pre_sig_up == current_sig_up) && (pre_sig_down == nullptr || pre_sig_down == current_sig_down) ||
                current_tr_up == nullptr) {
                if (first_insert == false && current_tr_up == nullptr || current_tr_up == nullptr) {
                    // std::cout << "continue" << std::endl;
                    continue;
                }
                // std::cout << "push" << std::endl;
                tr_up[i].push_back(current_tr_up);
                tr_down[i].push_back(current_tr_down);
                tr_shape_up[i].push_back(current_shape_up);
                tr_shape_down[i].push_back(current_shape_down);
                pre_sig_up = get_right_active(current_tr_up, current_shape_up);
                pre_sig_down = get_right_active(current_tr_down, current_shape_down);
                first_insert = true;
            } else if (pre_sig_up != current_sig_up || pre_sig_down != current_sig_down) {
                // std::cout << "insert null and cur_tr" << std::endl;
                tr_up[i].push_back(nullptr);
                tr_down[i].push_back(nullptr);
                tr_shape_up[i].push_back(2);
                tr_shape_down[i].push_back(2);
                pre_sig_up = nullptr;
                pre_sig_down = nullptr;

                tr_up[i].push_back(current_tr_up);
                tr_down[i].push_back(current_tr_down);
                tr_shape_up[i].push_back(current_shape_up);
                tr_shape_down[i].push_back(current_shape_down);
                pre_sig_up = get_right_active(current_tr_up, current_shape_up);
                pre_sig_down = get_right_active(current_tr_down, current_shape_down);
            } else {
                continue;
            }
        }
    }

    multirow_tr_permutation_up = tr_up;
    multirow_tr_permutation_down = tr_down;
    multirow_tr_shape_up = tr_shape_up;
    multirow_tr_shape_down = tr_shape_down;

    int max_col = 0;
    for (int i = 0; i < multirow_tr_permutation_up.size(); i++) {
        max_col = std::max(max_col, static_cast<int>(tr_up[i].size()));
    }

    for (int i = 0; i < row; i++) {
        while (max_col > multirow_tr_permutation_up[i].size()) {
            multirow_tr_permutation_up[i].push_back(nullptr);
            multirow_tr_permutation_down[i].push_back(nullptr);
            multirow_tr_shape_up[i].push_back(2);
            multirow_tr_shape_down[i].push_back(2);
        }
    }

    for (int i = 0; i < row; i++) {
        multirow_signal_permutation_up[i].assign(multirow_tr_permutation_up[0].size() * 2 + 1, nullptr);

        for (int j = 0; j < multirow_tr_permutation_up[i].size(); j++) {
            Transistor *tr_n = multirow_tr_permutation_up[i][j];
            if (tr_n) {
                switch (multirow_tr_shape_up[i][j]) {
                    case 0:
                        multirow_signal_permutation_up[i][2 * j] = tr_n->drain;
                        multirow_signal_permutation_up[i][2 * j + 1] = tr_n->gate;
                        multirow_signal_permutation_up[i][2 * j + 2] = tr_n->source;
                        break;
                    case 1:
                        multirow_signal_permutation_up[i][2 * j] = tr_n->source;
                        multirow_signal_permutation_up[i][2 * j + 1] = tr_n->gate;
                        multirow_signal_permutation_up[i][2 * j + 2] = tr_n->drain;
                        break;
                    case 2:
                        break;
                }
            }
        }

        multirow_signal_permutation_down[i].assign(multirow_tr_permutation_down[0].size() * 2 + 1, nullptr);

        for (int j = 0; j < multirow_tr_permutation_down[i].size(); j++) {
            Transistor *tr_p = multirow_tr_permutation_down[i][j];
            if (tr_p) {
                switch (multirow_tr_shape_down[i][j]) {
                    case 0:
                        multirow_signal_permutation_down[i][2 * j] = tr_p->drain;
                        multirow_signal_permutation_down[i][2 * j + 1] = tr_p->gate;
                        multirow_signal_permutation_down[i][2 * j + 2] = tr_p->source;
                        break;
                    case 1:
                        multirow_signal_permutation_down[i][2 * j] = tr_p->source;
                        multirow_signal_permutation_down[i][2 * j + 1] = tr_p->gate;
                        multirow_signal_permutation_down[i][2 * j + 2] = tr_p->drain;
                        break;
                    case 2:
                        break;
                }
            }
        }
    }

    print_pshape();
}

void Pshape::print_pshape() {
    const int row = multirow_tr_permutation_up.size();
    // print transistor name
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_tr_permutation_up[i].size(); j++) {
            if (multirow_tr_permutation_up[i][j] == nullptr) {
                std::cout << "        ";
            } else {
                std::cout << std::left << std::setw(7) << multirow_tr_permutation_up[i][j]->name << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "--" << std::endl;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_tr_permutation_down[i].size(); j++) {
            if (multirow_tr_permutation_down[i][j] == nullptr) {
                std::cout << "        ";
            } else {
                std::cout << std::left << std::setw(7) << multirow_tr_permutation_down[i][j]->name << " ";
            }
        }
        std::cout << std::endl;
    }
    // print active
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_tr_permutation_up[i].size(); j++) {
            Transistor *tr_n = multirow_tr_permutation_up[i][j];
            switch (multirow_tr_shape_up[i][j]) {
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
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_tr_permutation_up[i].size(); j++) {
            Transistor *tr_p = multirow_tr_permutation_down[i][j];
            switch (multirow_tr_shape_down[i][j]) {
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
    // for (int i = 0; i < row; i++) {
    //     for (int j = 0; j < multirow_signal_permutation_up[i].size(); j++) {
    //         Signal *sig = multirow_signal_permutation_up[i][j];
    //         if (sig == nullptr) {
    //             std::cout << "Null    ";
    //         } else {
    //             std::cout << std::left << std::setw(7) << sig->name << " ";
    //         }
    //     }
    // }
    // std::cout << std::endl;
    // for (int i = 0; i < row; i++) {
    //     for (int j = 0; j < multirow_signal_permutation_down[i].size(); j++) {
    //         Signal *sig = multirow_signal_permutation_down[i][j];
    //         if (sig == nullptr) {
    //             std::cout << "Null    ";
    //         } else {
    //             std::cout << std::left << std::setw(7) << sig->name << " ";
    //         }
    //     }
    // }
    // std::cout << std::endl;
}

bool Pshape::satisfy_via_rule() {
    std::cout << "hello satisfy_via_rule" << std::endl;
    const int row = multirow_tr_permutation_up.size();
    // assign available track case
    // VSS on the top
    z3::context ctx;
    z3::solver solver(ctx);
    z3::optimize opt(ctx);
    z3::expr_vector bool_vars(ctx);
    std::vector<std::vector<std::vector<z3::expr>>> via;
    std::vector<std::vector<z3::expr>> via_occupied;
    std::vector<Signal *> idx_to_sig;
    std::unordered_map<Signal *, int> sig_count;
    std::vector<std::unordered_map<Signal *, int>> sig_eachrow_count;
    std::unordered_map<Signal *, int> sig_crossrow_count;
    std::vector<std::vector<z3::expr>> max_y;
    std::vector<std::vector<z3::expr>> min_y;

    int idx = 0;
    calculate_available_track_case();

    int rows = row * 4;

    int cols = available_track_case[0].size();

    // assign id to signal
    for (auto pair : signals) {
        Signal *sig = pair.second;
        sig->id = idx;
        idx_to_sig.push_back(sig);
        idx++;
    }

    // count the number of row be occupied by signal
    sig_eachrow_count.resize(row);
    for (int i = 0; i < row; i++) {
        std::unordered_map<Signal *, bool> sig_visited;
        for (int j = 0; j < multirow_signal_permutation_up[i].size(); j++) {
            Signal *sig_up = multirow_signal_permutation_up[i][j];
            Signal *sig_down = multirow_signal_permutation_down[i][j];
            if (sig_up != nullptr && sig_down != nullptr) {
                if (sig_up == sig_down) {
                    if (sig_visited[sig_up] == false) {
                        sig_visited[sig_up] = true;
                        sig_crossrow_count[sig_up]++;
                    }
                    sig_eachrow_count[i][sig_up]++;
                } else {
                    if (sig_visited[sig_up] == false) {
                        sig_visited[sig_up] = true;
                        sig_crossrow_count[sig_up]++;
                    }
                    if (sig_visited[sig_down] == false) {
                        sig_visited[sig_down] = true;
                        sig_crossrow_count[sig_down]++;
                    }
                    sig_eachrow_count[i][sig_up]++;
                    sig_eachrow_count[i][sig_down]++;
                }
            }
        }
    }

    // count the number of grids occupied by signal
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_signal_permutation_up[i].size(); j++) {
            Signal *sig_up = multirow_signal_permutation_up[i][j];
            Signal *sig_down = multirow_signal_permutation_down[i][j];
            if (sig_up != nullptr && sig_down != nullptr) {
                if (sig_up == sig_down) {
                    sig_count[sig_up]++;
                } else {
                    sig_count[sig_up]++;
                    sig_count[sig_down]++;
                }
            }
        }
    }

    via.resize(signals.size());
    for (int s = 0; s < via.size(); s++) {
        via[s].assign(rows, std::vector<z3::expr>(cols, ctx.bool_val(false)));
    }

    for (int s = 0; s < signals.size(); s++) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                std::string var_name = "b_" + std::to_string(s) + "_" + std::to_string(i) + "_" + std::to_string(j);
                via[s][i][j] = ctx.bool_const(var_name.c_str());
            }
        }
    }

    max_y.resize(row);
    min_y.resize(row);
    for (int r = 0; r < row; r++) {
        max_y[r].assign(signals.size(), ctx.int_val(0));
        min_y[r].assign(signals.size(), ctx.int_val(0));
    }
    for (int r = 0; r < row; r++) {
        for (int s = 0; s < signals.size(); s++) {
            max_y[r][s] = ctx.int_const((std::to_string(r) + "_" + std::to_string(s) + "_max_y").c_str());
            min_y[r][s] = ctx.int_const((std::to_string(r) + "_" + std::to_string(s) + "_min_y").c_str());
            opt.add(max_y[r][s] >= min_y[r][s]);
            for (int i = r * 4; i < r * 4 + 4; i++) {
                z3::expr row_has_true = ctx.bool_val(false);
                for (int j = 0; j < via[s][i].size(); j++) {
                    row_has_true = row_has_true || via[s][i][j];
                }
                opt.add(z3::implies(row_has_true, max_y[r][s] >= i - r * 4));
                opt.add(z3::implies(row_has_true, min_y[r][s] <= i - r * 4));
            }
        }
    }

    z3::expr total_diff = ctx.int_val(0);  // 初始化為 0
    for (int r = 0; r < row; r++) {
        for (int s = 0; s < signals.size(); s++) {
            total_diff = total_diff + (max_y[r][s] - min_y[r][s]);
        }
    }
    opt.minimize(total_diff);

    via_occupied.resize(via[0].size());
    for (int i = 0; i < via[0].size(); i++) {
        via_occupied[i].assign(via[0][0].size(), ctx.bool_val(false));
    }
    for (int s = 0; s < via.size(); s++) {
        for (int i = 0; i < via[s].size(); i++) {
            for (int j = 0; j < via[s][i].size(); j++) {
                via_occupied[i][j] = via_occupied[i][j] || via[s][i][j];
            }
        }
    }

    // 加入約束：相鄰的方格不能同時為 true
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i > 0)  // 上方相鄰
                opt.add(!(via_occupied[i][j] && via_occupied[i - 1][j]));
            if (i < rows - 1)  // 下方相鄰
                opt.add(!(via_occupied[i][j] && via_occupied[i + 1][j]));
            if (j > 0)  // 左方相鄰
                opt.add(!(via_occupied[i][j] && via_occupied[i][j - 1]));
            if (j < cols - 1)  // 右方相鄰
                opt.add(!(via_occupied[i][j] && via_occupied[i][j + 1]));
        }
    }

    // assign via
    for (int s = 0; s < signals.size(); s++) {
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < available_track_case[i].size(); j++) {
                int track_case = available_track_case[i][j];
                // if signal on (i, j)
                if (multirow_signal_permutation_up[i][j] == idx_to_sig[s]) {
                    if (sig_count[idx_to_sig[s]] <= 1 && idx_to_sig[s]->is_io_pins == false) {  // no via needed
                        opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                    } else {  // via needed
                        switch (track_case) {
                            case 0:  // nullptr on (i, j)
                                opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                break;
                            case 1:
                                opt.add(z3::ite(via[s][4 * i][j], ctx.int_val(1), ctx.int_val(0)) +
                                            z3::ite(via[s][4 * i + 1][j], ctx.int_val(1), ctx.int_val(0)) +
                                            z3::ite(via[s][4 * i + 2][j], ctx.int_val(1), ctx.int_val(0)) +
                                            z3::ite(via[s][4 * i + 3][j], ctx.int_val(1), ctx.int_val(0)) ==
                                        ctx.int_val(1));

                                break;
                            case 2:
                                opt.add(z3::ite(via[s][4 * i][j], ctx.int_val(1), ctx.int_val(0)) +
                                            z3::ite(via[s][4 * i + 1][j], ctx.int_val(1), ctx.int_val(0)) +
                                            z3::ite(via[s][4 * i + 2][j], ctx.int_val(1), ctx.int_val(0)) +
                                            z3::ite(via[s][4 * i + 3][j], ctx.int_val(1), ctx.int_val(0)) ==
                                        ctx.int_val(1));

                                break;
                            case 3: {
                                Signal *sig_down = multirow_signal_permutation_down[i][j];
                                int s_paired = sig_down->id;
                                z3::expr p1 = via[s][4 * i + 1][j] && via[s_paired][4 * i + 3][j] && !via[s][4 * i][j] && !via[s][4 * i + 2][j] &&
                                              !via[s][4 * i + 3][j] && !via[s_paired][4 * i][j] && !via[s_paired][4 * i + 1][j] && !via[s_paired][4 * i + 2][j];
                                z3::expr p2 = via[s][4 * i][j] && via[s_paired][4 * i + 3][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] &&
                                              !via[s][4 * i + 3][j] && !via[s_paired][4 * i][j] && !via[s_paired][4 * i + 1][j] && !via[s_paired][4 * i + 2][j];
                                z3::expr p3 = via[s][4 * i + 2][j] && via[s_paired][4 * i][j] && !via[s][4 * i][j] && !via[s][4 * i + 1][j] &&
                                              !via[s][4 * i + 3][j] && !via[s_paired][4 * i + 1][j] && !via[s_paired][4 * i + 2][j] &&
                                              !via[s_paired][4 * i + 3][j];
                                z3::expr p4 = via[s][4 * i + 3][j] && via[s_paired][4 * i][j] && !via[s][4 * i][j] && !via[s][4 * i + 1][j] &&
                                              !via[s][4 * i + 2][j] && !via[s_paired][4 * i + 1][j] && !via[s_paired][4 * i + 2][j] &&
                                              !via[s_paired][4 * i + 3][j];
                                opt.add(z3::ite(p1, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p2, ctx.int_val(1), ctx.int_val(0)) +
                                            z3::ite(p3, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p4, ctx.int_val(1), ctx.int_val(0)) ==
                                        ctx.int_val(1));
                                break;
                            }
                            case 4: {
                                break;
                            }
                            case 5: {
                                opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                break;
                            }
                        }
                    }
                } else if (multirow_signal_permutation_down[i][j] == idx_to_sig[s]) {
                    if (sig_count[idx_to_sig[s]] <= 1 && idx_to_sig[s]->is_io_pins == false) {  // no via needed
                        opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                    } else {
                        switch (track_case) {
                            case 0:
                                opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                break;
                            case 1:
                                opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                break;
                            case 2:
                                opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                break;
                            case 3: {
                                Signal *sig_up = multirow_signal_permutation_up[i][j];
                                int s_paired = sig_up->id;
                                if (sig_count[sig_up] == 1) {
                                    z3::expr p1 = via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j] &&
                                                  !via[s_paired][4 * i][j] && !via[s_paired][4 * i + 1][j] && !via[s_paired][4 * i + 2][j] &&
                                                  !via[s_paired][4 * i + 3][j];
                                    z3::expr p2 = !via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && via[s][4 * i + 3][j] &&
                                                  !via[s_paired][4 * i][j] && !via[s_paired][4 * i + 1][j] && !via[s_paired][4 * i + 2][j] &&
                                                  !via[s_paired][4 * i + 3][j];
                                }
                                break;
                            }
                            case 4:
                                if (i % 2 == 0) {
                                    opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && via[s][4 * i + 3][j]);
                                } else {
                                    opt.add(via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                }
                                break;

                            case 5:
                                opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                break;
                        }
                    }
                } else {
                    opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                }
            }
        }
    }

    // reserve space
    for (int s = 0; s < signals.size(); s++) {
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < available_track_case[i].size(); j++) {
                Signal *sig = idx_to_sig[s];
                if ((multirow_signal_permutation_up[i][j] == idx_to_sig[s] || multirow_signal_permutation_down[i][j] == idx_to_sig[s]) &&
                    idx_to_sig[s]->name != "VDD" && idx_to_sig[s]->name != "VSS") {
                    if (sig->is_io_pins == true || sig_crossrow_count[sig] > 1 && sig_eachrow_count[i][sig] == 1) {
                        std::cout << "reserve space: " << sig->name << " " << i << " " << j << std::endl;
                        if (j == 0) {
                            for (int r = 0; r < 4; r++) {
                                opt.add(z3::implies(via[s][4 * i + r][j], !via_occupied[4 * i + r][j + 1] && !via_occupied[4 * i + r][j + 2]));
                            }
                        }
                        if (j == 1) {
                            for (int r = 0; r < 4; r++) {
                                opt.add(z3::implies(via[s][4 * i + r][j],
                                                    !via_occupied[4 * i + r][j - 1] && !via_occupied[4 * i + r][j + 1] && !via_occupied[4 * i + r][j + 2]));
                            }
                        }
                        if (j == available_track_case[i].size() - 2) {
                            for (int r = 0; r < 4; r++) {
                                opt.add(z3::implies(via[s][4 * i + r][j],
                                                    !via_occupied[4 * i + r][j - 2] && !via_occupied[4 * i + r][j - 1] && !via_occupied[4 * i + r][j + 1]));
                            }
                        }
                        if (j == available_track_case[i].size() - 1) {
                            for (int r = 0; r < 4; r++) {
                                opt.add(z3::implies(via[s][4 * i + r][j], !via_occupied[4 * i + r][j - 2] && !via_occupied[4 * i + r][j - 1]));
                            }
                        }
                    }
                }
            }
        }
    }
    if (opt.check() == z3::sat) {
        std::cout << "SAT" << std::endl;
        z3::model m = opt.get_model();
        for (int i = 0; i < via[0].size(); i++) {
            for (int j = 0; j < via[0][i].size(); j++) {
                std::cout << (m.eval(via_occupied[i][j]).bool_value() == Z3_L_TRUE ? "1 " : "0 ");
            }
            std::cout << std::endl;
        }

        // check correctness
        std::vector<std::vector<int>> matrix_constraint;
        matrix_constraint.resize(rows, std::vector<int>(cols, 0));
        for (int s = 0; s < signals.size(); s++) {
            for (int i = 0; i < via[s].size(); i++) {
                for (int j = 0; j < via[s][i].size(); j++) {
                    matrix_constraint[i][j] += (m.eval(via[s][i][j]).bool_value() == Z3_L_TRUE ? 1 : 0);
                }
            }
        }

        // print via of signal
        via_preassignment.assign(rows, std::vector<Signal *>(cols, nullptr));
        for (int s = 0; s < idx_to_sig.size(); s++) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    if (m.eval(via[s][y][x]).bool_value() == Z3_L_TRUE) {
                        via_preassignment[y][x] = idx_to_sig[s];
                    }
                }
            }
        }

        std::cout << "total_via_vertical_diff: " << m.eval(total_diff) << std::endl;
        for (int i = 0; i < via[0].size(); i++) {
            for (int j = 0; j < via[0][i].size(); j++) {
                assert(matrix_constraint[i][j] <= 1);
            }
        }
        return true;
    } else {
        std::cout << "UNSAT" << std::endl;
        return false;
    }
}

void Pshape::calculate_available_track_case() {
    int layout_width = 0;
    const int row = multirow_tr_permutation_up.size();
    for (int i = 0; i < row; i++) {
        layout_width = std::max(static_cast<int>(multirow_tr_permutation_up[i].size()), layout_width);
    }
    available_track_case.assign(row, std::vector<int>(layout_width * 2 + 1, 0));
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_signal_permutation_up[i].size(); j++) {
            Signal *sig_n = multirow_signal_permutation_up[i][j];
            Signal *sig_p = multirow_signal_permutation_down[i][j];
            if (sig_n != nullptr) {
                std::cout << sig_n->name << "/" << sig_p->name << " ";
            } else {
                std::cout << "NULL/NULL ";
            }
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
        std::cout << std::endl;
    }
    for (int i = 0; i < available_track_case.size(); i++) {
        for (int j = 0; j < available_track_case[i].size(); j++) {
            std::cout << available_track_case[i][j];
        }
        std::cout << std::endl;
    }
}

void Pshape::generate_multirow_plmt() {
    std::cout << "hello generate_multirow_plmt" << std::endl;
    const std::string folderName = "results";
    if (fs::exists(folderName)) fs::remove_all(folderName);
    if (!createFolder(folderName)) {
        std::cerr << "無法創建資料夾 " << folderName << "\n";
    }

    // 計算 header 要用的最大欄數
    int max_cols = 0;
    const int row = multirow_tr_permutation_up.size();
    for (int i = 0; i < row; i++) {
        max_cols = std::max(max_cols, static_cast<int>(multirow_tr_permutation_up[i].size()));
    }

    std::string fileName = folderName + "/output.plmt";
    std::ofstream ofs(fileName);
    if (!ofs) {
        std::cerr << "無法開啟檔案 " << fileName << "\n";
        return;
    }

    int cols = via_preassignment.empty() ? 0 : via_preassignment[0].size();

    // Header
    ofs << "<NAME> 0\n"
        << "<POWER> VDD\n"
        << "<GROUND> VSS\n"
        << "<OUTPUT> ";
    for (auto *sig : outputs) ofs << sig->name << " ";
    ofs << "\n<INPUT> ";
    for (auto *sig : inputs) ofs << sig->name << " ";
    ofs << "\n<ROWS> " << row << " <COLS> " << max_cols * 3 << "\n\n";

    // PMOS block
    ofs << "<PMOS>\n";
    for (size_t r = 0; r < row; ++r) {
        for (size_t c = 0; c < max_cols; ++c) {
            auto *tr = multirow_tr_permutation_down[r][c];
            int shape = multirow_tr_shape_down[r][c];
            if (shape == 0) {
                ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name;
            } else if (shape == 1) {
                ofs << tr->source->name << " <" << tr->gate->name << "> " << tr->drain->name;
            } else if (shape == 2 && c > 0 && c + 1 < max_cols) {
                auto *pre = multirow_tr_permutation_down[r][c - 1];
                auto *post = multirow_tr_permutation_down[r][c + 1];
                std::string left;
                std::string right;
                if (pre) {
                    left = (multirow_tr_shape_down[r][c - 1] == 0 ? pre->source->name : pre->drain->name);
                } else {
                    left = " <VDD> ";
                }
                if (post) {
                    right = (multirow_tr_shape_down[r][c + 1] == 0 ? post->drain->name : post->source->name);
                } else {
                    right = " <VDD> ";
                }
                ofs << left << " <VDD> " << right;
            } else {
                // fallback at edges
                auto *pre = multirow_tr_permutation_down[r][c - 1];
                std::string left;
                std::string right;
                if (pre) {
                    left = (multirow_tr_shape_down[r][c - 1] == 0 ? pre->source->name : pre->drain->name);
                } else {
                    left = " <VDD> ";
                }
                right = " <VDD> ";
                ofs << left << " <VDD> " << right;
            }
            if (c + 1 < max_cols) ofs << " ";
        }
        ofs << "\n";
    }
    ofs << "\n";
    // NMOS block
    ofs << "<NMOS>\n";
    for (size_t r = 0; r < row; ++r) {
        for (size_t c = 0; c < max_cols; ++c) {
            auto *tr = multirow_tr_permutation_up[r][c];
            int shape = multirow_tr_shape_up[r][c];
            if (shape == 0) {
                ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name;
            } else if (shape == 1) {
                ofs << tr->source->name << " <" << tr->gate->name << "> " << tr->drain->name;
            } else if (shape == 2 && c > 0 && c + 1 < max_cols) {
                auto *pre = multirow_tr_permutation_up[r][c - 1];
                auto *post = multirow_tr_permutation_up[r][c + 1];
                std::string left;
                std::string right;
                if (pre) {
                    left = (multirow_tr_shape_up[r][c - 1] == 0 ? pre->source->name : pre->drain->name);
                } else {
                    left = " <VSS> ";
                }
                if (post) {
                    right = (multirow_tr_shape_up[r][c + 1] == 0 ? post->drain->name : post->source->name);
                } else {
                    right = " <VSS> ";
                }
                ofs << left << " <VSS> " << right;
            } else {
                // fallback at edges
                if (tr) {
                    ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name;
                } else {
                    auto *pre = multirow_tr_permutation_up[r][c - 1];
                    std::string left;
                    std::string right;
                    if (pre) {
                        left = (multirow_tr_shape_up[r][c - 1] == 0 ? pre->source->name : pre->drain->name);
                    } else {
                        left = " <VSS> ";
                    }
                    right = " <VSS> ";
                    ofs << left << " <VSS> " << right;
                }
            }
            if (c + 1 < max_cols) ofs << " ";
        }
        ofs << "\n";
    }
    ofs << "\n";

    // VIA Preassignment block
    ofs << "<VIA_PREASSIGNMENT>\n";
    for (int i = 0; i < row * 4; ++i) {
        for (int j = 0; j < cols; ++j) {
            // 若 via_preassignment 尺寸不符，就輸出 0
            std::string v = (i < (int)via_preassignment.size() && j < (int)via_preassignment[i].size())
                                ? (via_preassignment[i][j]) ? via_preassignment[i][j]->name : "NULL"
                                : "NULL";
            ofs << v << ' ';
        }
        ofs << "\n";
    }
    ofs << "\n";

    ofs.close();
    std::cout << "Generated " << fileName << "\n";
}

int Pshape::inter_row_signal_count() {
    std::set<Signal *> signal_set;
    std::set<Signal *> interrow_signal_set;
    const int rows = multirow_tr_permutation_up.size();
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < multirow_tr_permutation_down[i].size(); j++) {
            Transistor *tr_p = multirow_tr_permutation_down[i][j];
            Transistor *tr_n = multirow_tr_permutation_up[i][j];
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
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < multirow_tr_permutation_down[i].size(); j++) {
                Transistor *tr_p = multirow_tr_permutation_down[i][j];
                Transistor *tr_n = multirow_tr_permutation_up[i][j];
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
        if (end != start) {
            interrow_signal_set.insert(sig);
        }
        result = result + end - start;
    }
    std::cout << "inter-row signal set:" << std::endl;
    for (auto sig : interrow_signal_set) {
        std::cout << sig->name << std::endl;
    }
    return interrow_signal_set.size();
}