#include <omp.h>
#include <sys/stat.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "cfet.h"
#include "z3++.h"

void Pshape::move_tr_to_left(int y, int x) {
    std::cout << "hello move_tr_to_left" << std::endl;
    Transistor *tr_up = multirow_tr_permutation_up[y][x];
    Transistor *tr_down = multirow_tr_permutation_down[y][x];
    Signal *drain_up = multirow_tr_permutation_up[y][x]->drain;
    Signal *source_up = multirow_tr_permutation_up[y][x]->source;
    Signal *drain_down = multirow_tr_permutation_down[y][x]->drain;
    Signal *source_down = multirow_tr_permutation_down[y][x]->source;
    for (int i = 0; i < multirow_tr_permutation_up.size(); i++) {
        auto pair = get_most_left_sig(i);
        Signal *sig_up = pair.first;
        Signal *sig_down = pair.second;
        bool sudu = (sig_up == drain_up);
        bool susu = (sig_up == source_up);
        bool sddd = (sig_down == drain_down);
        bool sdsd = (sig_down == source_down);
        if ((sudu || susu) && (sddd || sdsd)) {
            int shape_up = (sudu) ? 0 : 1;
            int shape_down = (sddd) ? 0 : 1;

            multirow_tr_permutation_up[i].push_back(tr_up);
            multirow_tr_permutation_down[i].push_back(tr_down);
            multirow_tr_shape_up[i].push_back(shape_up);
            multirow_tr_shape_down[i].push_back(shape_down);

            multirow_tr_permutation_up[y][x] = nullptr;
            multirow_tr_permutation_down[y][x] = nullptr;
            multirow_tr_shape_up[y][x] = 2;
            multirow_tr_shape_down[y][x] = 2;
            break;
        }
    }
    allign();
}

void Pshape::move_tr(int y1, int x1, int y2, int x2) {
    // std::cout << "hello move_tr" << std::endl;
    // std::cout << "[move] " << y1 << " " << x1 << " to " << y2 << " " << x2 << std::endl;

    Transistor *tr_up = multirow_tr_permutation_up[y1][x1];
    Transistor *tr_down = multirow_tr_permutation_down[y1][x1];
    assert(tr_up != nullptr || tr_down != nullptr);
    Signal *drain_up = (drain_up) ? multirow_tr_permutation_up[y1][x1]->drain : nullptr;
    Signal *source_up = (source_up) ? multirow_tr_permutation_up[y1][x1]->source : nullptr;
    Signal *drain_down = (drain_down) ? multirow_tr_permutation_down[y1][x1]->drain : nullptr;
    Signal *source_down = (source_down) ? multirow_tr_permutation_down[y1][x1]->source : nullptr;
    const int rows = multirow_tr_permutation_up.size();
    const int cols = multirow_tr_permutation_up[0].size();

    // std::cout << "rows: " << rows << " cols: " << cols << std::endl;

    std::vector<std::vector<Transistor *>> new_multirow_tr_permutation_up;
    std::vector<std::vector<Transistor *>> new_multirow_tr_permutation_down;
    std::vector<std::vector<int>> new_multirow_tr_shape_up;
    std::vector<std::vector<int>> new_multirow_tr_shape_down;
    new_multirow_tr_permutation_up.resize(rows);
    new_multirow_tr_permutation_down.resize(rows);
    new_multirow_tr_shape_up.resize(rows);
    new_multirow_tr_shape_down.resize(rows);
    multirow_tr_permutation_up[y1][x1] = nullptr;
    multirow_tr_permutation_down[y1][x1] = nullptr;
    multirow_tr_shape_up[y1][x1] = 2;
    multirow_tr_shape_down[y1][x1] = 2;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < std::max(cols, x2); j++) {
            if (i == y2 && j == x2) {
                auto pair_right = get_left_sig(i, j - 1);
                auto pair_left = get_right_sig(i, j);
                Signal *sig_up_left = pair_right.first;
                Signal *sig_down_left = pair_right.second;
                Signal *sig_up_right = pair_left.first;
                Signal *sig_down_right = pair_left.second;
                bool lsudu = (sig_up_left == drain_up || sig_up_left == nullptr || drain_up == nullptr);
                bool lsusu = (sig_up_left == source_up || sig_up_left == nullptr || source_up == nullptr);
                bool lsddd = (sig_down_left == drain_down || sig_down_right == nullptr || drain_down == nullptr);
                bool lsdsd = (sig_down_left == source_down || sig_down_left == nullptr || source_down == nullptr);
                bool rsudu = (sig_up_right == drain_up || sig_up_left == nullptr || drain_up == nullptr);
                bool rsusu = (sig_up_right == source_up || sig_up_right == nullptr || source_up == nullptr);
                bool rsddd = (sig_down_right == drain_down || sig_down_left == nullptr || drain_down == nullptr);
                bool rsdsd = (sig_down_right == source_down || sig_down_left == nullptr || source_down == nullptr);
                bool left_share = (lsudu || lsusu) && (lsddd || lsdsd);
                bool right_share = (rsudu || rsusu) && (rsddd || rsdsd);
                int shape_up = 0;
                int shape_down = 0;
                if (left_share) {
                    shape_up = (lsudu) ? 0 : 1;
                    shape_down = (lsddd) ? 0 : 1;
                } else if (right_share) {
                    shape_up = (rsusu) ? 0 : 1;
                    shape_down = (rsdsd) ? 0 : 1;
                }
                new_multirow_tr_permutation_up[i].push_back(tr_up);
                new_multirow_tr_permutation_down[i].push_back(tr_down);
                new_multirow_tr_shape_up[i].push_back(shape_up);
                new_multirow_tr_shape_down[i].push_back(shape_down);
            }
            Transistor *current_tr_up = multirow_tr_permutation_up[i][j];
            Transistor *current_tr_down = multirow_tr_permutation_down[i][j];
            int current_shape_up = multirow_tr_shape_up[i][j];
            int current_shape_down = multirow_tr_shape_down[i][j];
            if (current_tr_up || current_tr_down) {
                new_multirow_tr_permutation_up[i].push_back(current_tr_up);
                new_multirow_tr_permutation_down[i].push_back(current_tr_down);
                new_multirow_tr_shape_up[i].push_back(current_shape_up);
                new_multirow_tr_shape_down[i].push_back(current_shape_down);
            }
        }
    }
    if (x2 == cols) {
        auto pair_left = get_right_sig(y2, cols - 1);
        Signal *sig_up_left = pair_left.first;
        Signal *sig_down_left = pair_left.second;
        bool lsudu = (sig_up_left == drain_up);
        bool lsusu = (sig_up_left == source_up);
        bool lsddd = (sig_down_left == drain_down);
        bool lsdsd = (sig_down_left == source_down);
        bool left_share = (lsudu || lsusu) && (lsddd || lsdsd);
        int shape_up = 0;
        int shape_down = 0;
        if (left_share) {
            shape_up = (lsudu) ? 0 : 1;
            shape_down = (lsddd) ? 0 : 1;
        }
        new_multirow_tr_permutation_up[y2].push_back(tr_up);
        new_multirow_tr_permutation_down[y2].push_back(tr_down);
        new_multirow_tr_shape_up[y2].push_back(shape_up);
        new_multirow_tr_shape_down[y2].push_back(shape_down);
    }
    multirow_tr_permutation_up = new_multirow_tr_permutation_up;
    multirow_tr_permutation_down = new_multirow_tr_permutation_down;
    multirow_tr_shape_up = new_multirow_tr_shape_up;
    multirow_tr_shape_down = new_multirow_tr_shape_down;
    allign();
}

void Pshape::shift_row(int row) {
    std::vector<std::vector<int>> idx_groups;
    bool meet_spacing = true;
    int num_group = -1;
    for (int i = 0; i < multirow_tr_permutation_up[row].size(); i++) {
        Transistor *tr = multirow_tr_permutation_up[row][i];
        if (meet_spacing) {
            if (tr) {
                idx_groups.push_back(std::vector<int>());
                num_group++;
                idx_groups[num_group].push_back(i);
                meet_spacing = false;
            }
        } else {
            if (tr) {
                idx_groups[num_group].push_back(i);
            } else {
                meet_spacing = true;
            }
        }
    }
    // for (int i = 0; i < idx_groups.size(); i++) {
    //     for (int j = 0; j < idx_groups[i].size(); j++) {
    //         std::cout << idx_groups[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    this->allign();
    Pshape *best_pshape = this;
    int best_cost = this->cost();
    for (int i = 0; i < idx_groups.size(); i++) {
        for (int j = i + 1; j < idx_groups.size(); j++) {
            int left_idx = idx_groups[i][0];
            int right_idx = idx_groups[j][idx_groups[j].size() - 1];
            // std::cout << left_idx << " " << right_idx << std::endl;
            auto pair_left = get_left_sig(row, left_idx);
            auto pair_right = get_right_sig(row, right_idx);
            Signal *sig_left_up = pair_left.first;
            Signal *sig_left_down = pair_left.second;
            Signal *sig_right_up = pair_right.first;
            Signal *sig_right_down = pair_right.second;
            if ((sig_left_up == sig_right_up || sig_left_up == nullptr || sig_right_up == nullptr) &&
                (sig_left_down == sig_right_down || sig_left_down == nullptr || sig_right_down == nullptr)) {
                Pshape *new_pshape = this->copy();
                for (int n = 0; n < idx_groups[i].size(); n++) {
                    // std::cout << i << " " << j << " " << n << std::endl;
                    if (new_pshape->multirow_tr_permutation_up[row][left_idx] == nullptr) {
                        left_idx = left_idx - 1;
                        right_idx = right_idx - 1;
                    }
                    new_pshape->move_tr(row, left_idx, row, right_idx + 1);
                    int current_cost = new_pshape->cost();
                    if (current_cost < best_cost) {
                        best_cost = current_cost;
                        best_pshape = new_pshape->copy();
                    }
                }
            }
        }
    }
    multirow_tr_permutation_up = best_pshape->multirow_tr_permutation_up;
    multirow_tr_permutation_down = best_pshape->multirow_tr_permutation_down;
    multirow_tr_shape_up = best_pshape->multirow_tr_shape_up;
    multirow_tr_shape_down = best_pshape->multirow_tr_shape_down;
    multirow_signal_permutation_up = best_pshape->multirow_signal_permutation_up;
    multirow_signal_permutation_down = best_pshape->multirow_signal_permutation_down;
}

void Pshape::improve(int improved_y, int improved_x) {
    const int rows = multirow_tr_permutation_up.size();
    const int cols = multirow_tr_permutation_up[0].size();
    int best_cost = cost();
    int cost = best_cost;
    Pshape *best_pshape = this->copy();
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Pshape *new_pshape = this->copy();
            new_pshape->allign();
            new_pshape->move_tr(improved_y, improved_x, i, j);
            new_pshape->shift_row(i);
            cost = new_pshape->cost();
            if (cost < best_cost) {
                best_cost = cost;
                // std::cout << "[update best cost] cost = " << best_cost << std::endl;
                best_pshape = new_pshape->copy();
            }
        }
    }
    // std::cout << "[finish improve]" << std::endl;
    multirow_tr_permutation_up = best_pshape->multirow_tr_permutation_up;
    multirow_tr_permutation_down = best_pshape->multirow_tr_permutation_down;
    multirow_tr_shape_up = best_pshape->multirow_tr_shape_up;
    multirow_tr_shape_down = best_pshape->multirow_tr_shape_down;
    multirow_signal_permutation_up = best_pshape->multirow_signal_permutation_up;
    multirow_signal_permutation_down = best_pshape->multirow_signal_permutation_down;
    interrow_signal_set = best_pshape->interrow_signal_set;
    this->allign();
}