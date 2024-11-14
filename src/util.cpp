#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include "z3++.h"
#include "cfet.h"

using namespace z3;

std::vector<CFET::Signal*> CFET::get_right_active_singlerow(Transistor* tr, std::vector<std::vector<int>> config) {
    std::vector<Signal*> signals;
    signals.assign(2, nullptr);
    for (int i = 0; i < config.size() ; i++) {
        for (int j = 0 ; j < config[0].size() ; j++) {
            if (config[i][j] == 0) {
                signals[i] = (i == 0) ? tr_pairs[tr]->source : tr->source; // n on p
            }
            else if (config[i][j] == 1) {
                signals[i] = (i == 0) ? tr_pairs[tr]->drain : tr->drain; // n on p
            }
            else {
                signals[i] = signals[i];
            }
        }
    }
    return signals;
}

std::vector<CFET::Signal*> CFET::get_left_active_singlerow(Transistor* tr, std::vector<std::vector<int>> config) {
    std::vector<Signal*> signals;
    signals.assign(2, nullptr);
    for (int i = 0; i < config.size() ; i++) {
        for (int j = 0 ; j < config[0].size() ; j++) {
            if (config[i][j] == 0) {
                signals[i] = (i == 0) ? tr_pairs[tr]->drain : tr->drain;
                break;
            }
            else if (config[i][j] == 1) {
                signals[i] = (i == 0) ? tr_pairs[tr]->source : tr->source;
                break;
            }
            else {
                signals[i] = signals[i];
                break;
            }
        }
    }
    return signals;
}

bool CFET::diffusion_sharing_single_row(std::vector<std::vector<int>> config_left, std::vector<std::vector<int>> config_right, Transistor* tr_left, Transistor* tr_right) {
    std::vector<Signal* > tr_left_right_signals = get_right_active_singlerow(tr_left, config_left);
    std::vector<Signal* > tr_right_left_signals = get_left_active_singlerow(tr_right, config_right);
    return ((tr_left_right_signals[0] == tr_right_left_signals[0] || tr_left_right_signals[0] == nullptr || tr_right_left_signals[0] == nullptr)
        && (tr_left_right_signals[1] == tr_right_left_signals[1]
        || tr_left_right_signals[1] == nullptr
        || tr_right_left_signals[1] == nullptr));
}

CFET::Signal* CFET::get_right_active(Transistor* tr, int tr_shape_id) {
    if (tr_shape_id == 0) {
        return tr->source;
    }
    else if (tr_shape_id == 1) {
        return tr->drain;
    }
    else {
        return nullptr;
    }
}

CFET::Signal* CFET::get_left_active(Transistor* tr, int tr_shape_id) {
    if (tr_shape_id == 0) {
        return tr->drain;
    }
    else if (tr_shape_id == 1) {
        return tr->source;
    }
    else {
        return nullptr;
    }
}

bool CFET::merge_enable(Pshape* old_pshape, Lambda* new_lamb, Transistor* nmos, Transistor* pmos, int row, int col) {
    // check overlapped
    for (int i = row; i < row + new_lamb->config_up.size(); i++) {
        if (i >= old_pshape->multirow_tr_shape_up.size()) {
            break;
        }
        else {
            for (int j = col; j < col + new_lamb->config_up[i-row].size(); j++) {
                if (j >= old_pshape->multirow_tr_shape_up[i].size()) {
                    break;
                }
                else {
                    if (old_pshape->multirow_tr_shape_up[i][j] != 2 || old_pshape->multirow_tr_shape_down[i][j] != 2) {
                        return false;
                    }
                }
            }
        }
    }
    // std::cout << "check overlapped finish" << std::endl;
    // check diffusion sharing
    for (int i = row; i < row + new_lamb->config_up.size(); i++) {
        // std::cout << "check diffusion sharing " << i << std::endl;
        // std::cout << "old_pshape->multirow_tr_shape_up.size(): " << old_pshape->multirow_tr_shape_up.size() << std::endl;
        if (i >= old_pshape->multirow_tr_shape_up.size()) {
            return true;
        }
        else {
            // std::cout << "else " << i << " new_lamb->config_up.size(): " << new_lamb->config_up.size() << std::endl;
            Signal* up_old_signal;
            Signal* down_old_signal;
            // for (int r = 0; r < new_lamb->config_up.size(); r++) {
            //     for (int c = 0; c < new_lamb->config_up[r].size(); c++) {
            //         std::cout << new_lamb->config_up[r][c];
            //     }
            //     std::cout << std::endl;
            // }
            // std::cout << "if " << new_lamb->most_left_id[i - row] + col - 1 << std::endl;
            if (new_lamb->most_left_id[i - row] + col - 1 < old_pshape->multirow_tr_shape_up[i - row].size()) {
                up_old_signal = get_right_active(old_pshape->multirow_tr_permutation_up[i][new_lamb->most_left_id[i - row] + col - 1], old_pshape->multirow_tr_shape_up[i][new_lamb->most_left_id[i - row] + col - 1]);
                down_old_signal = get_right_active(old_pshape->multirow_tr_permutation_down[i][new_lamb->most_left_id[i - row] + col - 1], old_pshape->multirow_tr_shape_down[i][new_lamb->most_left_id[i - row] + col - 1]);
            }
            else {
                up_old_signal = nullptr;
                down_old_signal = nullptr;
            }
            // std::cout << (up_old_signal == nullptr) << " " << (down_old_signal == nullptr) << std::endl;
            Signal* up_new_signal = get_left_active(nmos, new_lamb->config_up[i - row][new_lamb->most_left_id[i - row]]);
            Signal* down_new_signal = get_left_active(pmos, new_lamb->config_down[i - row][new_lamb->most_left_id[i - row]]);
            // std::cout << up_new_signal->name << " " << down_new_signal->name << std::endl;
            // std::cout << ((up_old_signal == nullptr) ? "nullptr" : up_old_signal->name) << std::endl;
            // std::cout << ((up_new_signal == nullptr) ? "nullptr" : up_new_signal->name) << std::endl;
            // std::cout << ((down_old_signal == nullptr) ? "nullptr" : down_old_signal->name) << std::endl;
            // std::cout << ((down_new_signal == nullptr) ? "nullptr" : down_new_signal->name) << std::endl;
            if ((up_old_signal == up_new_signal || up_old_signal == nullptr || up_new_signal == nullptr)
                && (down_old_signal == down_new_signal || down_old_signal == nullptr || down_new_signal == nullptr)
            ) {
                continue;
            }
            else {
                return false;
            }
        }
    }
    return true;
}

CFET::Pshape* CFET::merge(Pshape* old_pshape, Lambda* new_lamb, Transistor* nmos, Transistor* pmos, int row, int col) {
    Pshape* pshape = new Pshape();
    int total_height = std::max(old_pshape->multirow_tr_shape_up.size(), new_lamb->config_up.size() + row);
    int total_width = std::max(old_pshape->multirow_tr_shape_up[0].size(), new_lamb->config_up[0].size() + col);
    // std::cout << "total_height/width: " << total_height << " " << total_width << std::endl;
    std::vector<std::vector<int>> new_tr_shape_up;
    std::vector<std::vector<int>> new_tr_shape_down;
    std::vector<std::vector<Transistor*>> new_tr_permutation_up;
    std::vector<std::vector<Transistor*>> new_tr_permutation_down;
    std::vector<int> new_most_right_idx;
    int new_height = 0;
    int new_multirow_area = 0;
    new_tr_shape_up.assign(total_height, std::vector<int>(total_width));
    new_tr_shape_down.assign(total_height, std::vector<int>(total_width));
    new_tr_permutation_up.assign(total_height, std::vector<Transistor*>(total_width));
    new_tr_permutation_down.assign(total_height, std::vector<Transistor*>(total_width));
    new_most_right_idx.assign(total_height, 0);
    pshape->width = 0;
    for (int i = 0; i < total_height; i++) {
        int width = -1;
        for (int j = 0; j < total_width; j++) {
            // std::cout << i << " " << j << std::endl;
            new_tr_shape_up[i][j] = 2;
            new_tr_shape_down[i][j] = 2;
            new_tr_permutation_up[i][j] = nullptr;
            new_tr_permutation_down[i][j] = nullptr;
            if (i < old_pshape->multirow_tr_shape_up.size() && j < old_pshape->multirow_tr_shape_up[0].size()) {
                new_tr_shape_up[i][j] = old_pshape->multirow_tr_shape_up[i][j];
                new_tr_shape_down[i][j] = old_pshape->multirow_tr_shape_down[i][j];
                new_tr_permutation_up[i][j] = old_pshape->multirow_tr_permutation_up[i][j];
                new_tr_permutation_down[i][j] = old_pshape->multirow_tr_permutation_down[i][j];
            }
            if (i >= row && j >= col) {
                if (i - row < new_lamb->config_up.size() && j - col < new_lamb->config_up[0].size()) {
                    if (new_lamb->config_up[i - row][j - col] != 2) {
                        new_tr_shape_up[i][j] = new_lamb->config_up[i - row][j - col];
                        new_tr_permutation_up[i][j] = nmos;
                    }
                    if (new_lamb->config_down[i - row][j - col] != 2) {
                        new_tr_shape_down[i][j] = new_lamb->config_down[i - row][j - col];
                        new_tr_permutation_down[i][j] = pmos;
                    }
                }
            }
            if (new_tr_shape_up[i][j] != 2 || new_tr_shape_down[i][j] != 2) {
                new_most_right_idx[i] = j;
                new_height = i + 1;
                width = j;
                if (width + 1 > pshape->width) {
                    pshape->width = width + 1;
                }
            }
            if (i == total_height - 1) {
                if (new_tr_shape_up[i][j] != 2 || new_tr_shape_down[i][j] != 2) {
                    pshape->top_width = j + 1;
                }
            }
        }
        if (width != -1) {
            // new_multirow_area = new_multirow_area + width + 1 + 2;
            new_multirow_area = new_multirow_area + width + 1;
        }
    }
    pshape->multirow_tr_shape_up = new_tr_shape_up;
    pshape->multirow_tr_shape_down = new_tr_shape_down;
    pshape->multirow_tr_permutation_up = new_tr_permutation_up;
    pshape->multirow_tr_permutation_down = new_tr_permutation_down;
    pshape->most_right_idx = new_most_right_idx;
    pshape->height = new_height;
    pshape->multirow_area = new_multirow_area;
    // pshape->multirow_macro_area = (pshape->width + 2) * pshape->height;
    pshape->multirow_macro_area = (pshape->width) * pshape->height;
    if (pshape->height <= max_allowable_cell_height) {
        // std::cout << "merged: macro area = " << pshape->multirow_macro_area << " top_width: " << pshape->top_width << std::endl;
        // for (int r = 0; r < pshape->multirow_tr_shape_up.size(); r++) {
        //     for (int c = 0; c < pshape->multirow_tr_shape_up[r].size(); c++) {
        //         std::cout << pshape->multirow_tr_shape_up[r][c];
        //     }
        //     std::cout << std::endl;
        // }
        // std::cout << "--" << std::endl;
        // for (int r = 0; r < pshape->multirow_tr_shape_down.size(); r++) {
        //     for (int c = 0; c < pshape->multirow_tr_shape_down[r].size(); c++) {
        //         std::cout << pshape->multirow_tr_shape_down[r][c];
        //     }
        //     std::cout << std::endl;
        // }
    }
    return pshape;
}