#include <omp.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
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

std::pair<Signal *, Signal *> Pshape::get_left_sig(int row, int col) {
    if (row > multirow_tr_permutation_up.size() - 1 || col > multirow_tr_permutation_up[0].size() - 1 || row < 0 || col < 0) {
        return std::make_pair(nullptr, nullptr);
    }
    Transistor *tr_up = multirow_tr_permutation_up[row][col];
    Transistor *tr_down = multirow_tr_permutation_down[row][col];
    if (tr_up) {
        Signal *sig_up = multirow_signal_permutation_up[row][col * 2];
        Signal *sig_down = multirow_signal_permutation_down[row][col * 2];
        return std::make_pair(sig_up, sig_down);
    } else {
        return std::make_pair(nullptr, nullptr);
    }
}

std::pair<Signal *, Signal *> Pshape::get_right_sig(int row, int col) {
    if (row > multirow_tr_permutation_up.size() - 1 || col > multirow_tr_permutation_up[0].size() - 1 || row < 0 || col < 0) {
        return std::make_pair(nullptr, nullptr);
    }
    Transistor *tr_up = multirow_tr_permutation_up[row][col];
    Transistor *tr_down = multirow_tr_permutation_down[row][col];
    if (tr_up) {
        Signal *sig_up = multirow_signal_permutation_up[row][col * 2 + 2];
        Signal *sig_down = multirow_signal_permutation_down[row][col * 2 + 2];
        return std::make_pair(sig_up, sig_down);
    } else {
        return std::make_pair(nullptr, nullptr);
    }
}

void Pshape::allign() {
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
    for (int i = 0; i < row; i++) {
        Signal *pre_sig_up = nullptr;
        Signal *pre_sig_down = nullptr;
        bool first_insert = false;
        for (int j = 0; j < multirow_tr_permutation_up[i].size(); j++) {
            Transistor *current_tr_up = multirow_tr_permutation_up[i][j];
            Transistor *current_tr_down = multirow_tr_permutation_down[i][j];
            int current_shape_up = multirow_tr_shape_up[i][j];
            int current_shape_down = multirow_tr_shape_down[i][j];
            Signal *current_sig_up = get_left_active(current_tr_up, current_shape_up);
            Signal *current_sig_down = get_left_active(current_tr_down, current_shape_down);
            // if able to abut
            if ((pre_sig_up == nullptr || current_sig_up == nullptr || pre_sig_up == current_sig_up) &&
                (pre_sig_down == nullptr || current_sig_down == nullptr || pre_sig_down == current_sig_down)) {
                // if meet spacing
                if (current_tr_up == nullptr && current_tr_down == nullptr) {
                    continue;
                }
                tr_up[i].push_back(current_tr_up);
                tr_down[i].push_back(current_tr_down);
                tr_shape_up[i].push_back(current_shape_up);
                tr_shape_down[i].push_back(current_shape_down);
                pre_sig_up = get_right_active(current_tr_up, current_shape_up);
                pre_sig_down = get_right_active(current_tr_down, current_shape_down);
                first_insert = true;
            }
            // if cannot abut
            else if (pre_sig_up != current_sig_up || pre_sig_down != current_sig_down) {
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
    multirow_signal_permutation_up.resize(row);
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
        multirow_signal_permutation_down.resize(row);
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
    // std::cout << "multirow_signal_permutation_down[0].size(): " << multirow_signal_permutation_down[0].size() << std::endl;
    // print_pshape();
}

void Pshape::fill_signal() {
    const int row = multirow_tr_permutation_up.size();
    const int cols = multirow_tr_permutation_up[0].size() * 2 + 1;
    multirow_signal_permutation_up.resize(row);
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
        multirow_signal_permutation_down.resize(row);
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
}

void Pshape::expand(int row, int col) {
    const int rows = multirow_tr_permutation_up.size();
    const int cols = multirow_tr_permutation_up[0].size();

    std::cout << "col: " << col << std::endl;
    for (int r = 0; r < rows; r++) {
        if (r == row) {
            multirow_tr_permutation_up[r].push_back(multirow_tr_permutation_up[r][cols - 1]);
            multirow_tr_permutation_down[r].push_back(multirow_tr_permutation_down[r][cols - 1]);
            multirow_tr_shape_up[r].push_back(multirow_tr_shape_up[r][cols - 1]);
            multirow_tr_shape_down[r].push_back(multirow_tr_shape_down[r][cols - 1]);
            for (int c = cols - 1; c >= col + 1; c--) {
                multirow_tr_permutation_up[r][c] = multirow_tr_permutation_up[r][c - 1];
                multirow_tr_permutation_down[r][c] = multirow_tr_permutation_down[r][c - 1];
                multirow_tr_shape_up[r][c] = multirow_tr_shape_up[r][c - 1];
                multirow_tr_shape_down[r][c] = multirow_tr_shape_down[r][c - 1];
            }
            multirow_tr_permutation_up[r][col] = nullptr;
            multirow_tr_permutation_down[r][col] = nullptr;
            multirow_tr_shape_up[r][col] = 2;
            multirow_tr_shape_down[r][col] = 2;
        } else {
            multirow_tr_permutation_up[r].push_back(nullptr);
            multirow_tr_permutation_down[r].push_back(nullptr);
            multirow_tr_shape_up[r].push_back(2);
            multirow_tr_shape_down[r].push_back(2);
        }
    }
    fill_signal();
    calculate_available_track_case();
}

void Pshape::print_pshape() {
    std::cout << "hello print_shape" << std::endl;
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
            if (tr_n == nullptr) {
                std::cout << "------------------------";
            } else {
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
        }
        std::cout << std::endl;
    }
    std::cout << "--" << std::endl;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_tr_permutation_down[i].size(); j++) {
            Transistor *tr_p = multirow_tr_permutation_down[i][j];
            if (tr_p == nullptr) {
                std::cout << "------------------------";
            } else {
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
        }
        std::cout << std::endl;
    }
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_signal_permutation_up[i].size(); j++) {
            Signal *sig = multirow_signal_permutation_up[i][j];
            if (sig == nullptr) {
                std::cout << "Null    ";
            } else {
                std::cout << std::left << std::setw(7) << sig->name << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_signal_permutation_down[i].size(); j++) {
            Signal *sig = multirow_signal_permutation_down[i][j];
            if (sig == nullptr) {
                std::cout << "Null    ";
            } else {
                std::cout << std::left << std::setw(7) << sig->name << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

bool Pshape::satisfy_via_rule(bool optimized) {
    fill_signal();
    calculate_available_track_case();
    const int row = multirow_tr_permutation_up.size();
    const int rows = row * 4;
    const int cols = available_track_case[0].size();
    // assign available track case
    // VSS on the top
    z3::context ctx;
    z3::solver solver(ctx);
    z3::optimize opt(ctx);
    std::vector<std::vector<std::vector<z3::expr>>> via;  // [s][r][c]
    std::vector<std::vector<z3::expr>> metal;             // [r][c]
    std::vector<std::vector<z3::expr>> via_occupied;
    std::vector<Signal *> idx_to_sig;
    std::unordered_map<Signal *, int> sig_count;
    std::vector<std::unordered_map<Signal *, int>> sig_eachrow_count;
    std::unordered_map<Signal *, int> sig_crossrow_count;
    std::vector<std::vector<z3::expr>> max_y;
    std::vector<std::vector<z3::expr>> min_y;
    std::vector<std::vector<z3::expr>> metal_density;
    std::vector<std::vector<z3::expr>> max_x;
    std::vector<std::vector<z3::expr>> min_x;
    std::vector<std::vector<z3::expr>> metal_occupied;
    std::vector<std::vector<z3::expr>> grid_id;

    int idx = 0;
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
                std::string name = "via_" + std::to_string(s) + "_" + std::to_string(i) + "_" + std::to_string(j);
                via[s][i][j] = ctx.bool_const(name.c_str());
            }
        }
    }

    metal.assign(rows, std::vector<z3::expr>(cols, ctx.bool_val(false)));

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            std::string name = "metal_" + std::to_string(i) + "_" + std::to_string(j);
            metal[i][j] = ctx.bool_const(name.c_str());
        }
    }
    grid_id.resize(rows);
    for (int r = 0; r < grid_id.size(); r++) {
        grid_id[r].assign(cols, ctx.int_val(false));
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            std::string name = "grid_id_" + std::to_string(i) + "_" + std::to_string(j);
            grid_id[i][j] = ctx.int_const(name.c_str());
        }
    }
    metal_occupied.resize(rows);
    for (int r = 0; r < metal_occupied.size(); r++) {
        metal_occupied[r].assign(cols, ctx.bool_val(false));
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            std::string name = "metal_occupied_" + std::to_string(i) + "_" + std::to_string(j);
            metal_occupied[i][j] = ctx.bool_const(name.c_str());
        }
    }

    // assign grid_id
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            z3::expr via_occu = ctx.bool_val(false);
            for (int s = 0; s < signals.size(); s++) {
                via_occu = via_occu || (via[s][r][c]);
                opt.add(z3::implies(via[s][r][c], grid_id[r][c] == s));
            }
            // opt.add(z3::implies(!via_occu, grid_id[r][c] == -1));
            opt.add(z3::implies(via_occu, metal[r][c]));
        }
    }

    // grid_id connectivity
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (c == 0) {
                opt.add((metal[r][c] && metal[r][c + 1]) == (grid_id[r][c] == grid_id[r][c + 1]));
            } else if (c == cols - 1) {
                opt.add((metal[r][c] && metal[r][c - 1]) == (grid_id[r][c] == grid_id[r][c - 1]));
            } else {
                opt.add((metal[r][c] && metal[r][c + 1]) == (grid_id[r][c] == grid_id[r][c + 1]));
                opt.add((metal[r][c] && metal[r][c - 1]) == (grid_id[r][c] == grid_id[r][c - 1]));
            }
        }
    }

    std::vector<std::vector<z3::expr>> gL(rows, std::vector<z3::expr>(cols, ctx.bool_val(false)));
    std::vector<std::vector<z3::expr>> gR(rows, std::vector<z3::expr>(cols, ctx.bool_val(false)));

    for (int y = 0; y < rows; ++y) {
        for (int z = 0; z < cols; ++z) {
            std::string name = "gR_" + std::to_string(y) + "_" + std::to_string(z);
            gR[y][z] = ctx.bool_const(name.c_str());
        }
    }

    for (int y = 0; y < rows; ++y) {
        for (int z = 0; z < cols; ++z) {
            std::string name = "gL_" + std::to_string(y) + "_" + std::to_string(z);
            gL[y][z] = ctx.bool_const(name.c_str());
        }
    }
    // gL assignment
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (x == 0) {
                opt.add(gL[y][x] == metal[y][x]);
            } else {
                opt.add(gL[y][x] == (!metal[y][x - 1] && metal[y][x]));
            }
        }
    }
    // gR assignment
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (x == cols - 1) {
                opt.add(gR[y][x] == metal[y][x]);
            } else {
                opt.add(gR[y][x] == (metal[y][x] && !metal[y][x + 1]));
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
        }
    }

    for (int r = 0; r < row; r++) {
        for (int s = 0; s < signals.size(); s++) {
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

    max_x.resize(rows);
    min_x.resize(rows);

    for (int r = 0; r < rows; r++) {
        max_x[r].assign(signals.size(), ctx.int_val(0));
        min_x[r].assign(signals.size(), ctx.int_val(0));
    }

    for (int r = 0; r < rows; r++) {
        for (int s = 0; s < signals.size(); s++) {
            max_x[r][s] = ctx.int_const((std::to_string(r) + "_" + std::to_string(s) + "_max_x").c_str());
            min_x[r][s] = ctx.int_const((std::to_string(r) + "_" + std::to_string(s) + "_min_x").c_str());
        }
    }

    for (int r = 0; r < rows; r++) {
        for (int s = 0; s < signals.size(); s++) {
            opt.add(max_x[r][s] >= min_x[r][s]);
            for (int c = 0; c < cols; c++) {
                opt.add(z3::implies(via[s][r][c], max_x[r][s] >= c));
                opt.add(z3::implies(via[s][r][c], min_x[r][s] <= c));
            }
        }
    }
    metal_density.resize(rows);
    for (int i = 0; i < rows; i++) {
        metal_density[i].assign(cols, ctx.int_val(0));
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            metal_density[i][j] = ctx.int_const((std::to_string(i) + "_" + std::to_string(j) + "_metal_density").c_str());
        }
    }

    for (int s = 0; s < signals.size(); s++) {
        Signal *sig = idx_to_sig[s];
        if (sig_count[sig] <= 1 && sig->is_io_pins == false || sig->name == "VSS" || sig->name == "VDD") {
            continue;
        }
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                z3::expr metal_cover = (c <= max_x[r][s]) && (c >= min_x[r][s]);
                metal_density[r][c] = metal_density[r][c] + z3::ite(metal_cover, ctx.int_val(1), ctx.int_val(0));
            }
        }
    }

    via_occupied.resize(via[0].size());
    for (int i = 0; i < via[0].size(); i++) {
        via_occupied[i].assign(via[0][0].size(), ctx.bool_val(false));
    }

    for (int i = 0; i < via[0].size(); i++) {
        for (int j = 0; j < via[0][0].size(); j++) {
            via_occupied[i][j] = ctx.bool_const((std::to_string(i) + "_" + std::to_string(j) + "_via_occupied").c_str());
        }
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
                        // std::cout << idx_to_sig[s]->name << " " << s << " up at (" << i << ", " << j << ") track_case: " << track_case << std::endl;
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
                                if (sig_count[sig_down] != 1) {
                                    z3::expr p1 = via[s][4 * i + 1][j] && via[s_paired][4 * i + 3][j] && !via[s][4 * i][j] && !via[s][4 * i + 2][j] &&
                                                  !via[s][4 * i + 3][j] && !via[s_paired][4 * i][j] && !via[s_paired][4 * i + 1][j] &&
                                                  !via[s_paired][4 * i + 2][j];
                                    z3::expr p2 = via[s][4 * i][j] && via[s_paired][4 * i + 3][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] &&
                                                  !via[s][4 * i + 3][j] && !via[s_paired][4 * i][j] && !via[s_paired][4 * i + 1][j] &&
                                                  !via[s_paired][4 * i + 2][j];
                                    z3::expr p3 = via[s][4 * i + 2][j] && via[s_paired][4 * i][j] && !via[s][4 * i][j] && !via[s][4 * i + 1][j] &&
                                                  !via[s][4 * i + 3][j] && !via[s_paired][4 * i + 1][j] && !via[s_paired][4 * i + 2][j] &&
                                                  !via[s_paired][4 * i + 3][j];
                                    z3::expr p4 = via[s][4 * i + 3][j] && via[s_paired][4 * i][j] && !via[s][4 * i][j] && !via[s][4 * i + 1][j] &&
                                                  !via[s][4 * i + 2][j] && !via[s_paired][4 * i + 1][j] && !via[s_paired][4 * i + 2][j] &&
                                                  !via[s_paired][4 * i + 3][j];
                                    opt.add(z3::ite(p1, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p2, ctx.int_val(1), ctx.int_val(0)) +
                                                z3::ite(p3, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p4, ctx.int_val(1), ctx.int_val(0)) ==
                                            ctx.int_val(1));
                                } else {
                                    opt.add(z3::ite(via[s][4 * i][j], ctx.int_val(1), ctx.int_val(0)) +
                                                z3::ite(via[s][4 * i + 1][j], ctx.int_val(1), ctx.int_val(0)) +
                                                z3::ite(via[s][4 * i + 2][j], ctx.int_val(1), ctx.int_val(0)) +
                                                z3::ite(via[s][4 * i + 3][j], ctx.int_val(1), ctx.int_val(0)) ==
                                            ctx.int_val(1));
                                }
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
                        // std::cout << idx_to_sig[s]->name << " " << s << " down at (" << i << ", " << j << ") track_case: " << track_case << std::endl;
                        switch (track_case) {
                            case 0:
                                opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                break;
                            case 1: {
                                Signal *sig_up = multirow_signal_permutation_up[i][j];
                                if (sig_up == nullptr) {
                                    opt.add(z3::ite(via[s][4 * i][j], ctx.int_val(1), ctx.int_val(0)) +
                                                z3::ite(via[s][4 * i + 1][j], ctx.int_val(1), ctx.int_val(0)) +
                                                z3::ite(via[s][4 * i + 2][j], ctx.int_val(1), ctx.int_val(0)) +
                                                z3::ite(via[s][4 * i + 3][j], ctx.int_val(1), ctx.int_val(0)) ==
                                            ctx.int_val(1));
                                } else {
                                    opt.add(!via[s][4 * i][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j]);
                                }

                                break;
                            }
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
                                    opt.add(z3::ite(p1, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p2, ctx.int_val(1), ctx.int_val(0)) == ctx.int_val(1));
                                } else {
                                    z3::expr p1 = via[s_paired][4 * i + 1][j] && via[s][4 * i + 3][j] && !via[s_paired][4 * i][j] &&
                                                  !via[s_paired][4 * i + 2][j] && !via[s_paired][4 * i + 3][j] && !via[s][4 * i][j] && !via[s][4 * i + 1][j] &&
                                                  !via[s][4 * i + 2][j];
                                    z3::expr p2 = via[s_paired][4 * i][j] && via[s][4 * i + 3][j] && !via[s_paired][4 * i + 1][j] &&
                                                  !via[s_paired][4 * i + 2][j] && !via[s_paired][4 * i + 3][j] && !via[s][4 * i][j] && !via[s][4 * i + 1][j] &&
                                                  !via[s][4 * i + 2][j];
                                    z3::expr p3 = via[s_paired][4 * i + 2][j] && via[s][4 * i][j] && !via[s_paired][4 * i][j] && !via[s_paired][4 * i + 1][j] &&
                                                  !via[s_paired][4 * i + 3][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j];
                                    z3::expr p4 = via[s_paired][4 * i + 3][j] && via[s][4 * i][j] && !via[s_paired][4 * i][j] && !via[s_paired][4 * i + 1][j] &&
                                                  !via[s_paired][4 * i + 2][j] && !via[s][4 * i + 1][j] && !via[s][4 * i + 2][j] && !via[s][4 * i + 3][j];
                                    opt.add(z3::ite(p1, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p2, ctx.int_val(1), ctx.int_val(0)) +
                                                z3::ite(p3, ctx.int_val(1), ctx.int_val(0)) + z3::ite(p4, ctx.int_val(1), ctx.int_val(0)) ==
                                            ctx.int_val(1));
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

    // MAR rule (MAR = 1)
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (c == 0) {
                opt.add(!(metal[r][c] && !metal[r][c + 1]));
            } else if (c == cols - 1) {
                opt.add(!(metal[r][c] && !metal[r][c - 1]));
            } else {
                opt.add(!(metal[r][c] && !metal[r][c + 1] && !metal[r][c - 1]));
            }
        }
    }

    // PRL rule (PRL = 1)
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (y == 0) {
                opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y + 1][x], ctx.int_val(1), ctx.int_val(0)) <= 1);
            } else if (y == rows - 1) {
                opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y - 1][x], ctx.int_val(1), ctx.int_val(0)) <= 1);
            } else {
                opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y + 1][x], ctx.int_val(1), ctx.int_val(0)) <= 1);
                opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y - 1][x], ctx.int_val(1), ctx.int_val(0)) <= 1);
            }
        }
    }

    // SHR rule (SHR = 2)
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (x == 0) {
                if (y == 0) {
                    opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gR[y + 1][x + 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                } else if (y == rows - 1) {
                    opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gR[y - 1][x + 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                } else {
                    opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gR[y - 1][x + 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                    opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gR[y + 1][x + 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                }
            } else if (x == cols - 1) {
                if (y == 0) {
                    opt.add(z3::ite(gL[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y + 1][x - 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                } else if (y == rows - 1) {
                    opt.add(z3::ite(gL[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y - 1][x - 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                } else {
                    opt.add(z3::ite(gL[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y + 1][x - 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                    opt.add(z3::ite(gL[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y - 1][x - 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                }
            } else {
                if (y == 0) {
                    opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gR[y + 1][x + 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                    opt.add(z3::ite(gL[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y + 1][x - 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                } else if (y == rows - 1) {
                    opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gR[y - 1][x + 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                    opt.add(z3::ite(gL[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y - 1][x - 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                } else {
                    opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gR[y - 1][x + 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                    opt.add(z3::ite(gR[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gR[y + 1][x + 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                    opt.add(z3::ite(gL[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y + 1][x - 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                    opt.add(z3::ite(gL[y][x], ctx.int_val(1), ctx.int_val(0)) + z3::ite(gL[y - 1][x - 1], ctx.int_val(1), ctx.int_val(0)) <= 1);
                }
            }
        }
    }

    z3::expr total_diff = ctx.int_val(0);
    for (int r = 0; r < row; r++) {
        for (int s = 0; s < signals.size(); s++) {
            total_diff = total_diff + (max_y[r][s] - min_y[r][s]);
        }
    }

    z3::expr total_density = ctx.int_val(0);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            // total_density = total_density + ite(metal_density[r][c] == 0, ctx.int_val(0), ite(metal_density[r][c] == 1, ctx.int_val(1), ctx.int_val(10)));
            total_density = total_density + ite(metal_density[r][c] == 0, ctx.int_val(0), ite(metal_density[r][c] == 1, ctx.int_val(1), ctx.int_val(10)));
        }
    }

    z3::expr total_metal = ctx.int_val(0);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            total_metal = total_metal + z3::ite(metal[r][c], ctx.int_val(1), ctx.int_val(0));
        }
    }

    if (optimized) {
        std::cout << "optimized" << std::endl;
        opt.minimize(total_diff);
        opt.minimize(total_density);
        opt.minimize(total_metal);
    }

    z3::params p(ctx);
    p.set("timeout", static_cast<unsigned>(600000));
    opt.set(p);
    std::cout << "before sat" << std::endl;
    z3::check_result result = opt.check();

    if (result == z3::sat) {
        std::cout << "SAT" << std::endl;
        z3::model m = opt.get_model();
        std::cout << "[via]" << std::endl;
        for (int i = 0; i < via[0].size(); i++) {
            for (int j = 0; j < via[0][i].size(); j++) {
                std::cout << (m.eval(via_occupied[i][j]).bool_value() == Z3_L_TRUE ? "1 " : "0 ");
            }
            std::cout << std::endl;
        }

        std::cout << "[metal]" << std::endl;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (m.eval(metal[i][j]).bool_value() == Z3_L_TRUE) {
                    std::cout << std::right << std::setw(3) << m.eval(grid_id[i][j]) << " ";
                } else {
                    std::cout << " -1 ";
                }
            }
            std::cout << std::endl;
        }

        std::cout << "[metal density]" << std::endl;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                std::cout << m.eval(metal_density[i][j]) << " ";
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

        std::cout << "[verticall diff]" << std::endl;
        for (int s = 0; s < signals.size(); s++) {
            std::cout << idx_to_sig[s]->name << std::endl;
            for (int r = 0; r < row; r++) {
                std::cout << "row " << r << ": " << m.eval(min_y[r][s]) << " " << m.eval(max_y[r][s]) << std::endl;
            }
        }

        m0_metal_fill.resize(rows);
        for (int i = 0; i < rows; i++) {
            m0_metal_fill[i].assign(cols, nullptr);
        }
        std::cout << "[satisfy via metal]" << std::endl;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int sid = m.eval((grid_id[r][c])).get_numeral_int();
                std::cout << ((m.eval(metal[r][c]).bool_value() == Z3_L_TRUE) ? 1 : 0) << "";
                m0_metal_fill[r][c] =
                    ((m.eval(metal[r][c]).bool_value() == Z3_L_TRUE) ? (sid >= 0 && sid < signals.size()) ? idx_to_sig[sid] : nullptr : nullptr);
            }
            std::cout << std::endl;
        }

        std::cout << "total_via_vertical_diff: " << m.eval(total_diff) << std::endl;
        std::cout << "total density: " << m.eval(total_density) << std::endl;

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                assert(matrix_constraint[i][j] <= 1);
            }
        }
        std::cout << "finish satisfy via rule" << std::endl;
        return true;
    } else if (result == z3::unknown) {
        std::cout << "⚠️ 超時或無法判定（unknown）" << std::endl;
        z3::model m = opt.get_model();
        std::cout << "[via]" << std::endl;
        for (int i = 0; i < via[0].size(); i++) {
            for (int j = 0; j < via[0][i].size(); j++) {
                std::cout << (m.eval(via_occupied[i][j]).bool_value() == Z3_L_TRUE ? "1 " : "0 ");
            }
            std::cout << std::endl;
        }

        std::cout << "[metal]" << std::endl;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (m.eval(metal[i][j]).bool_value() == Z3_L_TRUE) {
                    std::cout << std::right << std::setw(3) << m.eval(grid_id[i][j]) << " ";
                } else {
                    std::cout << " -1 ";
                }
            }
            std::cout << std::endl;
        }

        std::cout << "[metal density]" << std::endl;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                std::cout << m.eval(metal_density[i][j]) << " ";
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

        // std::cout << "[verticall diff]" << std::endl;
        // for (int s = 0; s < signals.size(); s++) {
        //     std::cout << idx_to_sig[s]->name << std::endl;
        //     for (int r = 0; r < row; r++) {
        //         std::cout << "row " << r << ": " << m.eval(min_y[r][s]) << " " << m.eval(max_y[r][s]) << std::endl;
        //     }
        // }

        m0_metal_fill.resize(rows);
        for (int i = 0; i < rows; i++) {
            m0_metal_fill[i].assign(cols, nullptr);
        }
        std::cout << "[satisfy via metal]" << std::endl;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int sid = m.eval((grid_id[r][c])).get_numeral_int();
                std::cout << ((m.eval(metal[r][c]).bool_value() == Z3_L_TRUE) ? 1 : 0) << "";
                m0_metal_fill[r][c] =
                    ((m.eval(metal[r][c]).bool_value() == Z3_L_TRUE) ? (sid >= 0 && sid < signals.size()) ? idx_to_sig[sid] : nullptr : nullptr);
            }
            std::cout << std::endl;
        }

        std::cout << "total_via_vertical_diff: " << m.eval(total_diff) << std::endl;
        std::cout << "total density: " << m.eval(total_density) << std::endl;

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
    std::cout << "hello calculate_available_track_case" << std::endl;
    int layout_width = 0;
    const int row = multirow_tr_permutation_up.size();
    for (int i = 0; i < row; i++) {
        layout_width = std::max(static_cast<int>(multirow_tr_permutation_up[i].size()), layout_width);
    }
    std::cout << "layout_width * 2 + 1: " << layout_width * 2 + 1 << std::endl;
    available_track_case.assign(row, std::vector<int>(layout_width * 2 + 1, 0));
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < multirow_signal_permutation_up[i].size(); j++) {
            Signal *sig_n = multirow_signal_permutation_up[i][j];
            Signal *sig_p = multirow_signal_permutation_down[i][j];

            if (sig_n == nullptr && sig_p == nullptr) {
                available_track_case[i][j] = 0;
            } else if ((sig_p == nullptr || sig_n == nullptr) || sig_n->name == sig_p->name && sig_n->name != "VSS" && sig_p->name != "VDD") {
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
}

void create_plmt_file(const std::string &cell_name) {
    const std::string folderName = "results";
    if (!fs::exists(folderName)) {
        fs::create_directory(folderName);
    }

    std::string fileName = folderName + "/" + cell_name + ".plmt";

    // 建立一個空白檔案（或覆蓋原有內容）
    std::ofstream ofs(fileName);
    if (!ofs) {
        std::cerr << "無法建立檔案：" << fileName << std::endl;
        return;
    }

    std::cout << "成功建立檔案：" << fileName << std::endl;
    ofs.close();  // 可視情況留下 open 狀態供後續寫入
}

void Pshape::generate_multirow_plmt() {
    // std::cout << "hello generate_multirow_plmt" << std::endl;
    const std::string folderName = "results";

    // 計算 header 要用的最大欄數
    int max_cols = 0;
    const int row = multirow_tr_permutation_up.size();
    for (int i = 0; i < row; i++) {
        max_cols = std::max(max_cols, static_cast<int>(multirow_tr_permutation_up[i].size()));
    }

    std::string fileName = folderName + "/" + cell_name + ".plmt";
    create_plmt_file(cell_name);
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
    ofs << "\n<ROWS> " << row << " <COLS> " << max_cols * 2 + 1 << "\n\n";
    // PMOS block
    ofs << "<PMOS>\n";

    for (int r = 0; r < row; ++r) {
        for (int c = 0; c < max_cols; ++c) {
            auto *tr = multirow_tr_permutation_down[r][c];
            int shape = multirow_tr_shape_down[r][c];
            // spacing
            if (tr == nullptr) {
                ofs << " Null Null Null ";
            } else if (shape == 0) {
                ofs << " " << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name << " ";
            } else if (shape == 1) {
                ofs << " " << tr->source->name << " <" << tr->gate->name << "> " << tr->drain->name << " ";
            }
            ofs << " ";
        }
        ofs << "\n";
    }
    ofs << "\n";
    std::cout << "max_cols: " << max_cols << std::endl;
    // NMOS block
    ofs << "<NMOS>\n";
    for (size_t r = 0; r < row; ++r) {
        for (size_t c = 0; c < max_cols; ++c) {
            auto *tr = multirow_tr_permutation_up[r][c];
            int shape = multirow_tr_shape_up[r][c];
            // spacing
            if (tr == nullptr) {
                ofs << " Null Null Null ";
            } else if (shape == 0) {
                ofs << " " << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name << " ";
            } else if (shape == 1) {
                ofs << " " << tr->source->name << " <" << tr->gate->name << "> " << tr->drain->name << " ";
            }
            ofs << " ";
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

    // M0 Metal fill
    ofs << "<M0 Metal fill>\n";
    for (int i = 0; i < row * 4; ++i) {
        for (int j = 0; j < cols; ++j) {
            // 若 via_preassignment 尺寸不符，就輸出 0
            std::string v =
                (i < (int)m0_metal_fill.size() && j < (int)m0_metal_fill[i].size()) ? (m0_metal_fill[i][j]) ? m0_metal_fill[i][j]->name : "NULL" : "NULL";
            ofs << v << ' ';
        }
        ofs << "\n";
    }
    ofs << "\n";

    ofs.close();
    std::cout << "Generated " << fileName << "\n";
}

int Pshape::inter_row_signal_count(bool print) {
    std::set<Signal *> signal_set;
    interrow_signal_set.clear();
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
    // for (auto sig : interrow_signal_set) {
    //     std::cout << "[inter-row signal] " << sig->name << std::endl;
    // }
    if (print) {
        std::cout << "#inter-row signals: " << interrow_signal_set.size() << std::endl;
    }

    return interrow_signal_set.size();
}

int Pshape::calculate_hpml(bool print) {
    // std::cout << "hello calculate_hpml" << std::endl;
    int hpml = 0;
    std::unordered_map<Signal *, int> min_x;
    std::unordered_map<Signal *, int> max_x;
    std::unordered_map<Signal *, int> min_y;
    std::unordered_map<Signal *, int> max_y;
    const int rows = multirow_signal_permutation_up.size();
    const int cols = multirow_signal_permutation_up[0].size();
    for (auto pair : signals) {
        Signal *sig = pair.second;
        if (sig) {
            min_x[sig] = cols;
            max_x[sig] = 0;
            min_y[sig] = rows;
            max_y[sig] = 0;
        }
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Signal *sig_up = multirow_signal_permutation_up[i][j];
            Signal *sig_down = multirow_signal_permutation_down[i][j];
            if (sig_up) {
                min_x[sig_up] = std::min(j, min_x[sig_up]);
                max_x[sig_up] = std::max(j, max_x[sig_up]);
                min_y[sig_up] = std::min(i, min_y[sig_up]);
                max_y[sig_up] = std::max(i, max_y[sig_up]);
            }
            if (sig_down) {
                min_x[sig_down] = std::min(j, min_x[sig_down]);
                max_x[sig_down] = std::max(j, max_x[sig_down]);
                min_y[sig_down] = std::min(i, min_y[sig_down]);
                max_y[sig_down] = std::max(i, max_y[sig_down]);
            }
        }
    }
    for (auto pair : signals) {
        Signal *sig = pair.second;
        if (sig && sig->name != "VDD" && sig->name != "VSS") {
            // std::cout << sig->name << " [" << max_x[sig] - min_x[sig] + max_y[sig] - min_y[sig] << "] " << max_x[sig] << " " << min_x[sig] << " " <<
            // max_y[sig]
            //           << " " << min_y[sig] << std::endl;
            hpml += max_x[sig] - min_x[sig] + max_y[sig] - min_y[sig];
        }
    }
    if (print) {
        std::cout << "hpml: " << hpml << std::endl;
    }
    this->hpml = hpml;
    return hpml;
}

int Pshape::calculate_area(bool print) {
    int area = multirow_tr_permutation_up.size() * multirow_tr_permutation_up[0].size();
    if (print) {
        std::cout << "area: " << area << std::endl;
    }
    return area;
}

int Pshape::calculate_metal_density(bool print) {
    int total_density = 0;
    const int rows = multirow_signal_permutation_up.size();
    const int cols = multirow_signal_permutation_up[0].size();
    std::vector<std::unordered_map<Signal *, int>> min_x;
    std::vector<std::unordered_map<Signal *, int>> max_x;
    std::unordered_map<Signal *, int> signal_count;
    std::vector<std::vector<int>> metal_density;
    metal_density.resize(rows);
    for (int r = 0; r < rows; r++) {
        metal_density[r].assign(cols, 0);
    }
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            Signal *sig_up = multirow_signal_permutation_up[r][c];
            Signal *sig_down = multirow_signal_permutation_down[r][c];
            if (sig_up) {
                if (sig_up == sig_down) {
                    signal_count[sig_up]++;
                } else {
                    signal_count[sig_up]++;
                    signal_count[sig_down]++;
                }
            }
        }
    }
    for (auto pair : signals) {
        Signal *sig = pair.second;
        if (sig && sig->name != "VSS" && sig->name != "VDD") {
            min_x.resize(rows);
            max_x.resize(rows);
            for (int r = 0; r < rows; r++) {
                min_x[r][sig] = cols;
                max_x[r][sig] = 0;
            }
        }
    }
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            Signal *sig_up = multirow_signal_permutation_up[r][c];
            Signal *sig_down = multirow_signal_permutation_down[r][c];
            if (sig_up) {
                min_x[r][sig_up] = std::min(c, min_x[r][sig_up]);
                max_x[r][sig_up] = std::max(c, max_x[r][sig_up]);
            }
            if (sig_down) {
                min_x[r][sig_down] = std::min(c, min_x[r][sig_down]);
                max_x[r][sig_down] = std::max(c, max_x[r][sig_down]);
            }
        }
    }
    for (int r = 0; r < rows; r++) {
        for (auto pair : signals) {
            Signal *sig = pair.second;
            if (signal_count[sig] == 1) {
                continue;
            }
            // std::cout << r << " " << sig->name << ": " << min_x[r][sig] << "/" << max_x[r][sig] << std::endl;
            if (min_x[r][sig] <= max_x[r][sig]) {
                for (int c = min_x[r][sig]; c <= max_x[r][sig]; c++) {
                    metal_density[r][c]++;
                }
            }
        }
    }
    // std::cout << "[metal density]" << std::endl;
    for (int r = 0; r < rows; r++) {
        for (int c = 1; c < cols; c++) {
            // std::cout << metal_density[r][c] << " ";
            total_density += metal_density[r][c] * metal_density[r][c];
        }
        // std::cout << std::endl;
    }
    if (print) {
        std::cout << "total density: " << total_density << std::endl;
    }
    return total_density;
}

int Pshape::cost(bool print) {
    int area = calculate_area(print);
    int hpml = calculate_hpml(print);
    int interrow_count = inter_row_signal_count(print);
    int metal_density = calculate_metal_density(print);
    int cost = alpha * area + beta * hpml + gamma * interrow_count + delta * metal_density;
    if (print) {
        std::cout << "cost: " << cost << std::endl;
        print_pshape();
    }
    return cost;
}

Pshape *Pshape::copy() {
    Pshape *new_pshape = new Pshape();
    new_pshape->multirow_tr_permutation_up = multirow_tr_permutation_up;
    new_pshape->multirow_tr_permutation_down = multirow_tr_permutation_down;
    new_pshape->multirow_tr_shape_up = multirow_tr_shape_up;
    new_pshape->multirow_tr_shape_down = multirow_tr_shape_down;
    new_pshape->multirow_signal_permutation_up = multirow_signal_permutation_up;
    new_pshape->multirow_signal_permutation_down = multirow_signal_permutation_down;
    new_pshape->interrow_signal_set = interrow_signal_set;
    return new_pshape;
}

std::vector<std::pair<int, int>> Pshape::choose_most_improved_target() {
    struct Grid {
        int y;
        int x;
        double score;
    };
    std::unordered_map<Signal *, std::vector<std::pair<int, int>>> sig_map;
    std::vector<Grid> most_improved_grid;
    std::vector<std::pair<int, int>> result;
    double max_improved = 0;
    for (int i = 0; i < multirow_signal_permutation_up.size(); i++) {
        for (int j = 0; j < multirow_signal_permutation_up[i].size(); j++) {
            Signal *sig_up = multirow_signal_permutation_up[i][j];
            Signal *sig_down = multirow_signal_permutation_down[i][j];
            if (sig_up) {
                sig_map[sig_up].push_back(std::make_pair(i, j));
                sig_map[sig_down].push_back(std::make_pair(i, j));
            }
        }
    }
    for (auto pair : signals) {
        Signal *sig = pair.second;
        // for (auto sig : interrow_signal_set) {
        if (sig->name == "VDD" || sig->name == "VSS") {
            continue;
        }
        for (int i = 0; i < sig_map[sig].size(); i++) {
            int total_dis = 0;
            std::pair<int, int> current_grid = sig_map[sig][i];
            int cur_y = current_grid.first;
            int cur_x = current_grid.second;
            if (cur_x != 0 && cur_x % 2 == 0 && multirow_signal_permutation_up[cur_y][cur_x - 1] && multirow_signal_permutation_up[cur_y][cur_x + 1]) {
                continue;
            }
            for (int j = 0; j < sig_map[sig].size(); j++) {
                std::pair<int, int> other_grid = sig_map[sig][j];
                int other_y = other_grid.first;
                int other_x = other_grid.second;
                total_dis = total_dis + std::abs(cur_y - other_y) + std::abs(cur_x - other_x);
            }
            double improved = static_cast<double>(total_dis) / sig_map[sig].size() / sig_map[sig].size();
            // std::cout << "[update most improved] " << sig->name << " grid: " << cur_y << " " << cur_x << " "
            // << "improved: " << improved << std::endl;total_via_vertical_diff:

            most_improved_grid.push_back(Grid{cur_y, cur_x, improved});
        }
    }
    std::sort(most_improved_grid.begin(), most_improved_grid.end(), [](const Grid &g1, const Grid &g2) { return g1.score > g2.score; });
    for (int i = 0; i < most_improved_grid.size(); i++) {
        Grid g = most_improved_grid[i];
        result.push_back(std::make_pair(g.y, g.x));
    }
    return result;
}
