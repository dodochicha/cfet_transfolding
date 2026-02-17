#include <omp.h>
#include <sys/stat.h>

#include <algorithm>
#include <cassert>
#include <chrono>
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
#include <unordered_set>
#include <utility>
#include <vector>

#include "cfet.h"

std::pair<std::vector<Node *>, std::vector<Pshape *>> bfs_placement(int target_area, std::vector<Transistor *> pmos_group) {
    std::vector<Pshape *> placement_cand;
    std::cout << "target_area: " << target_area << std::endl;
    std::queue<Node *> q;
    Node *root = new Node();
    std::vector<Transistor *> remained_pmos(pmos_group);
    root->name = "root";
    root->remained_pmos = remained_pmos;
    root->tr_left_sum = tr_size_sum;
    Transistor *root_tr = new Transistor();
    root_tr->name = "root_tr";
    root->tr = root_tr;
    q.push(root);
    while (q.empty() == false) {
        if (q.size() >= num_nodes_parsed_to_gpu) {
            std::vector<Node *> node_to_be_processed;
            while (q.empty() == false) {
                Node *n = q.front();
                node_to_be_processed.push_back(n);
                q.pop();
            }
            return std::make_pair(node_to_be_processed, std::vector<Pshape *>());
        }
        if (placement_cand.size() >= max_placement_size) {
            break;
        }
        Node *current_node = q.front();
        // branch
        // std::cout << "current_node->remained_pmos.size(): " << current_node->remained_pmos.size() << std::endl;
        if (current_node->remained_pmos.size() != 0) {
            // #pragma omp parallel for schedule(dynamic)
            for (Transistor *br_tr : current_node->remained_pmos) {
                Node *br_node = new Node();
                std::vector<Transistor *> remained_pmos(current_node->remained_pmos);
                for (auto it = remained_pmos.begin(); it != remained_pmos.end();) {
                    if (*it == br_tr) {
                        it = remained_pmos.erase(it);  // 刪除元素並返回新的位置
                    } else {
                        ++it;
                    }
                }
                br_node->parent = current_node;
                br_node->remained_pmos = remained_pmos;
                br_node->tr = br_tr;
                br_node->tr_left_sum = current_node->tr_left_sum - std::max(br_tr->num_finger, tr_pairs[br_tr]->num_finger);
                // create partial shape
                if (current_node->name == "root") {
                    std::vector<Pshape *> partial_placement;
                    for (auto lamb : multi_row_configs[br_tr]) {
                        Pshape *pshape = new Pshape();
                        // multi-row
                        pshape->multirow_area = std::max(br_tr->num_finger, tr_pairs[br_tr]->num_finger);
                        pshape->multirow_tr_shape_up = lamb->config_up;
                        pshape->multirow_tr_shape_down = lamb->config_down;
                        pshape->multirow_tr_permutation_up.assign(lamb->config_up.size(), std::vector<Transistor *>(lamb->config_up[0].size()));
                        pshape->multirow_tr_permutation_down.assign(lamb->config_down.size(), std::vector<Transistor *>(lamb->config_down[0].size()));
                        pshape->height = 0;
                        pshape->width = 0;
                        pshape->top_width = 0;
                        for (int row = 0; row < lamb->config_up.size(); row++) {
                            pshape->most_right_idx.push_back(0);
                            for (int col = 0; col < lamb->config_up[row].size(); col++) {
                                if (lamb->config_up[row][col] != 2) {
                                    pshape->multirow_tr_permutation_up[row][col] = tr_pairs[br_tr];
                                    pshape->most_right_idx[row] = col;
                                    pshape->height = row + 1;
                                } else {
                                    pshape->multirow_tr_permutation_up[row][col] = nullptr;
                                }
                                if (lamb->config_down[row][col] != 2) {
                                    pshape->multirow_tr_permutation_down[row][col] = br_tr;
                                    pshape->most_right_idx[row] = col;
                                    pshape->height = row + 1;
                                } else {
                                    pshape->multirow_tr_permutation_down[row][col] = nullptr;
                                }
                                if (lamb->config_up[row][col] != 2 || lamb->config_down[row][col] != 2) {
                                    if (col + 1 > pshape->width) {
                                        pshape->width = col + 1;
                                    }
                                }
                            }
                            if (row == lamb->config_up.size() - 1) {
                                for (int col = 0; col < lamb->config_up[row].size(); col++) {
                                    if (lamb->config_up[row][col] != 2 || lamb->config_down[row][col] != 2) {
                                        pshape->top_width = col + 1;
                                    }
                                }
                            }
                        }
                        pshape->multirow_macro_area = (pshape->width) * pshape->height;
                        // for (int i = 0; i < pshape->multirow_tr_permutation_up.size(); i++) {
                        //     for (int j = 0; j < pshape->multirow_tr_permutation_up[i].size(); j++) {
                        //         if (pshape->multirow_tr_permutation_up[i][j] == nullptr) std::cout << "  x ";
                        //         else std::cout << pshape->multirow_tr_permutation_up[i][j]->name << " ";
                        //     }
                        //     std::cout << std::endl;
                        // }
                        // std::cout << "--" << std::endl;
                        // for (int i = 0; i < pshape->multirow_tr_permutation_down.size(); i++) {
                        //     for (int j = 0; j < pshape->multirow_tr_permutation_down[i].size(); j++) {
                        //         if (pshape->multirow_tr_permutation_down[i][j] == nullptr) std::cout << "  x ";
                        //         else std::cout << pshape->multirow_tr_permutation_down[i][j]->name << " ";
                        //     }
                        //     std::cout << std::endl;
                        // }
                        // std::cout << std::endl;
                        partial_placement.push_back(pshape);
                    }
                    // assign partial_placement
                    br_node->partial_shapes = partial_placement;
                } else {
                    int min_potential_macro_area = std::numeric_limits<int>::max();
                    std::vector<Pshape *> new_partial_placement;
                    for (auto old_pshape : current_node->partial_shapes) {
                        for (auto lamb : multi_row_configs[br_tr]) {
                            auto new_tr_permutation = old_pshape->tr_permutaton;
                            auto new_tr_shape_id = old_pshape->tr_shape_id;
                            auto new_ds_array = old_pshape->ds_array;
                            for (int row = std::max(0, static_cast<int>(old_pshape->height - lamb->config_up.size())); row <= old_pshape->height; row++) {
                                int col;
                                if (row < old_pshape->multirow_tr_shape_up.size()) {
                                    col = old_pshape->most_right_idx[row];
                                } else {
                                    col = 0;
                                }
                                while (true) {
                                    if (merge_enable(old_pshape, lamb, tr_pairs[br_tr], br_tr, row, col)) {
                                        Pshape *pshape = merge(old_pshape, lamb, tr_pairs[br_tr], br_tr, row, col);
                                        if (pshape->height > max_allowable_cell_height) {
                                            if (pshape) delete pshape;
                                            pshape = nullptr;
                                            break;
                                        }
                                        int low_bound = std::numeric_limits<int>::max();
                                        for (int h = pshape->height; h <= max_allowable_cell_height; h++) {
                                            int _low_bound = std::max(pshape->width, (pshape->top_width + br_node->tr_left_sum + (h - pshape->height + 1) - 1) /
                                                                                         (h - pshape->height + 1));
                                            _low_bound = _low_bound * h;
                                            if (_low_bound < low_bound) {
                                                low_bound = _low_bound;
                                            }
                                        }
                                        // for (int i = 0; i < pshape->multirow_tr_permutation_up.size(); i++) {
                                        //     for (int j = 0; j < pshape->multirow_tr_permutation_up[i].size(); j++) {
                                        //         if (pshape->multirow_tr_permutation_up[i][j] == nullptr) std::cout << "  x ";
                                        //         else std::cout << pshape->multirow_tr_permutation_up[i][j]->name << " ";
                                        //     }
                                        //     std::cout << std::endl;
                                        // }
                                        // std::cout << "--" << std::endl;
                                        // for (int i = 0; i < pshape->multirow_tr_permutation_down.size(); i++) {
                                        //     for (int j = 0; j < pshape->multirow_tr_permutation_down[i].size(); j++) {
                                        //         if (pshape->multirow_tr_permutation_down[i][j] == nullptr) std::cout << "  x ";
                                        //         else std::cout << pshape->multirow_tr_permutation_down[i][j]->name << " ";
                                        //     }
                                        //     std::cout << std::endl;
                                        // }
                                        // std::cout << "current_node->tr_left_sum: " << current_node->tr_left_sum << std::endl;
                                        // std::cout << "low_bound: " << low_bound << std::endl;
                                        // std::cout << std::endl;
                                        if (low_bound > target_area) {
                                            if (pshape) delete pshape;
                                            pshape = nullptr;
                                            break;
                                        }
                                        if (low_bound < min_potential_macro_area) {
                                            // for (auto item : new_partial_placement) {
                                            //     if (item) delete item;
                                            //     item = nullptr;
                                            // }
                                            // new_partial_placement.clear();
                                            // new_partial_placement.shrink_to_fit();
                                            for (size_t i = 0; i < new_partial_placement.size(); ++i) {
                                                delete new_partial_placement[i];
                                                new_partial_placement[i] = nullptr;
                                            }
                                            new_partial_placement.clear();
                                            min_potential_macro_area = low_bound;
                                            new_partial_placement.push_back(pshape);
                                        } else if (low_bound == min_potential_macro_area) {
                                            new_partial_placement.push_back(pshape);
                                        } else {
                                            if (pshape) delete pshape;
                                            pshape = nullptr;
                                        }
                                        break;
                                    }
                                    col++;
                                }
                            }
                        }
                    }
                    br_node->partial_shapes = new_partial_placement;
                }
                if (br_node->partial_shapes.size() != 0) {
                    q.push(br_node);
                } else {
                    if (br_node) delete br_node;
                    br_node = nullptr;
                }
            }

            for (auto pshape : current_node->partial_shapes) {
                if (pshape) delete pshape;
                pshape = nullptr;
            }
            // delete current_node;
            // current_node = nullptr;
        } else {  // leaf
            for (auto pshape : current_node->partial_shapes) {
                if (placement_cand.size() < max_placement_size) {
                    if (pshape->multirow_macro_area <= target_area) {
                        // std::cout << "pshape->multirow_macro_area: " << pshape->multirow_macro_area << std::endl;
                        // for (int i = 0; i < pshape->multirow_tr_permutation_up.size(); i++) {
                        //     for (int j = 0; j < pshape->multirow_tr_permutation_up[i].size(); j++) {
                        //         std::cout << pshape->multirow_tr_permutation_up[i][j]->name << " ";
                        //     }
                        //     std::cout << std::endl;
                        // }
                        // std::cout << "--" << std::endl;
                        // for (int i = 0; i < pshape->multirow_tr_permutation_down.size(); i++) {
                        //     for (int j = 0; j < pshape->multirow_tr_permutation_down[i].size(); j++) {
                        //         std::cout << pshape->multirow_tr_permutation_down[i][j]->name << " ";
                        //     }
                        //     std::cout << std::endl;
                        // }
                        placement_cand.push_back(pshape);
                    } else {
                        if (pshape) delete pshape;
                        pshape = nullptr;
                    }
                } else {
                    break;
                }
            }
            // delete current_node;
            // current_node = nullptr;
        }
        // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
        if (placement_cand.size() >= max_placement_size) {
            break;
        }
        q.pop();
    }
    return std::make_pair(std::vector<Node *>(), placement_cand);
}

std::vector<Pshape *> dfs_placement(Node *root_node, int target_area, std::vector<Transistor *> pmos_group) {
    // std::cout << "dfs" << std::endl;
    // auto start_dfs = std::chrono::high_resolution_clock::now();
    std::vector<Pshape *> final_placement_cand;
    int min_macro_area = std::numeric_limits<int>::max() - 1;
    std::vector<Pshape *> placement_cand;
    bool pruned = false;
    Node *root = root_node->parent;
    std::vector<Transistor *> remained_pmos(root_node->remained_pmos);
    Transistor *current_tr = root_node->tr;
    Node *current_node = root_node;

    if (root_node->remained_pmos.size() == 0) {
        return root_node->partial_shapes;
    }

    while (true) {
        if (final_placement_cand.size() >= max_placement_size) {
            break;
        }
        if (pruned) {
            // std::cout << "pruned!" << std::endl;
            pruned = false;
            while (true) {
                Node *n = current_node->parent;
                for (auto pshape : current_node->partial_shapes) {
                    if (pshape) delete pshape;
                }
                if (current_node) delete current_node;
                current_node = n;
                current_tr = n->tr;
                if (((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) ==
                    false) {
                    break;
                }
            }
            current_node->fill++;
            if (current_node == root) {
                break;
            }
        } else if (current_node->remained_pmos.size() == 0) {
            // std::cout << "leaf!" << std::endl;
            if (current_node->partial_shapes.size() != 0) {
                for (auto pshape : current_node->partial_shapes) {
                    if (pshape->multirow_macro_area < min_macro_area) {
                        min_macro_area = pshape->multirow_macro_area;
                        for (auto item : placement_cand) {
                            if (item) delete item;
                            item = nullptr;
                        }
                        placement_cand.clear();
                        placement_cand.shrink_to_fit();
                        placement_cand.push_back(pshape);
                    } else if (pshape->multirow_macro_area == min_macro_area) {
                        placement_cand.push_back(pshape);
                    } else {
                        if (pshape) delete pshape;
                        pshape = nullptr;
                    }
                    if (placement_cand.size() >= max_placement_size) {
                        break;
                    }
                }
                // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
            } else {
                if (current_node) delete current_node;
            }
            if (placement_cand.size() >= max_placement_size) {
                break;
            }
            while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
                Node *n = current_node->parent;
                // tr_left_sum = tr_left_sum + std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                current_node = n;
                current_tr = n->tr;
            }
            current_node->fill++;
            if (current_node == root) {
                break;
            }
        } else {
            int min_potential_macro_area = std::numeric_limits<int>::max();
            if (current_node->partial_shapes.size() == 0) {
                current_node->partial_shapes.clear();
                current_node->partial_shapes.shrink_to_fit();
                pruned = true;
                continue;
            }
            Node *n = new Node();
            std::vector<Transistor *> remained_pmos = current_node->remained_pmos;
            Transistor *old_tr = current_tr;
            current_tr = remained_pmos[current_node->fill];
            std::vector<Pshape *> new_partial_placement;
            int low_bound;
            int tr_left_sum = current_node->tr_left_sum - std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
            for (auto old_pshape : current_node->partial_shapes) {
                // std::cout << "old_pshape: " << std::endl;
                // for (int r = 0; r < old_pshape->multirow_tr_shape_up.size(); r++) {
                //     for (int c = 0; c < old_pshape->multirow_tr_shape_up[r].size(); c++) {
                //         std::cout << old_pshape->multirow_tr_shape_up[r][c];
                //     }
                //     std::cout << std::endl;
                // }
                // std::cout << "--" << std::endl;
                // for (int r = 0; r < old_pshape->multirow_tr_shape_down.size(); r++) {
                //     for (int c = 0; c < old_pshape->multirow_tr_shape_down[r].size(); c++) {
                //         std::cout << old_pshape->multirow_tr_shape_down[r][c];
                //     }
                //     std::cout << std::endl;
                // }
                // std::cout << "multi_row_configs[current_tr].size(): " << multi_row_configs[current_tr].size() << std::endl;
                for (auto lamb : multi_row_configs[current_tr]) {
                    auto new_tr_permutation = old_pshape->tr_permutaton;
                    auto new_tr_shape_id = old_pshape->tr_shape_id;
                    auto new_ds_array = old_pshape->ds_array;
                    // std::cout << "new_lamb: " << std::endl;
                    // for (int r = 0; r < lamb->config_up.size(); r++) {
                    //     for (int c = 0; c < lamb->config_up[r].size(); c++) {
                    //         std::cout << lamb->config_up[r][c];
                    //     }
                    //     std::cout << std::endl;
                    // }
                    // std::cout << "--" << std::endl;
                    // for (int r = 0; r < lamb->config_down.size(); r++) {
                    //     for (int c = 0; c < lamb->config_down[r].size(); c++) {
                    //         std::cout << lamb->config_down[r][c];
                    //     }
                    //     std::cout << std::endl;
                    // }
                    for (int row = std::max(0, static_cast<int>(old_pshape->height - lamb->config_up.size())); row <= old_pshape->height; row++) {
                        int col;
                        if (row < old_pshape->multirow_tr_shape_up.size()) {
                            col = old_pshape->most_right_idx[row];
                        } else {
                            col = 0;
                        }
                        while (true) {
                            if (merge_enable(old_pshape, lamb, tr_pairs[current_tr], current_tr, row, col)) {
                                // std::cout << "row/col: " << row << " " << col << std::endl;
                                Pshape *pshape = merge(old_pshape, lamb, tr_pairs[current_tr], current_tr, row, col);
                                // cut by low_bound
                                // low_bound = pshape->multirow_area + tr_left_sum;
                                // low_bound = std::max(pshape->top_width + tr_left_sum, pshape->width) * pshape->height;
                                // low_bound = std::max(pshape->top_width + 2 + tr_left_sum, pshape->width + 2) * pshape->height;
                                low_bound = std::numeric_limits<int>::max();
                                for (int h = pshape->height; h <= max_allowable_cell_height; h++) {
                                    int _low_bound =
                                        std::max(pshape->width, (pshape->top_width + tr_left_sum + (h - pshape->height + 1) - 1) / (h - pshape->height + 1));
                                    _low_bound = _low_bound * h;
                                    // std::cout << "_low_bound: " << _low_bound << " h: " << h << std::endl;
                                    if (_low_bound < low_bound) {
                                        low_bound = _low_bound;
                                    }
                                }
                                // std::cout << "low_bound: " << low_bound << " tr_left_sum: " << tr_left_sum << std::endl;
                                // << min_potential_macro_area << std::endl; std::cout << "min_cell_area: " << min_cell_area << std::endl; std::cout
                                // << pshape->height << "/ " << max_allowable_cell_height << std::endl; std::cout << low_bound << " " <<
                                // min_cell_area + 1 << std::endl;
                                if (low_bound > target_area) {
                                    if (pshape) delete pshape;
                                    pshape = nullptr;
                                    break;
                                }
                                if (pshape->height > max_allowable_cell_height) {
                                    if (pshape) delete pshape;
                                    pshape = nullptr;
                                    break;
                                }
                                if (low_bound < min_potential_macro_area) {
                                    for (auto item : new_partial_placement) {
                                        if (item) delete item;
                                        item = nullptr;
                                    }
                                    new_partial_placement.clear();
                                    new_partial_placement.shrink_to_fit();
                                    min_potential_macro_area = low_bound;
                                    new_partial_placement.push_back(pshape);
                                } else if (low_bound == min_potential_macro_area) {
                                    new_partial_placement.push_back(pshape);
                                } else {
                                    if (pshape) delete pshape;
                                    pshape = nullptr;
                                }
                                break;
                            }
                            col++;
                        }
                    }
                }
            }
            n->partial_shapes = new_partial_placement;
            // remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), current_tr), remained_pmos.end());
            for (auto it = remained_pmos.begin(); it != remained_pmos.end();) {
                if (*it == current_tr) {
                    it = remained_pmos.erase(it);  // 刪除元素並返回新的位置
                } else {
                    ++it;
                }
            }

            n->remained_pmos = remained_pmos;
            n->tr_left_sum = tr_left_sum;
            n->parent = current_node;
            n->tr = current_tr;
            n->fill = 0;
            current_node->children.push_back(n);
            current_node = n;
        }
    }
    // auto end_dfs = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> elapsed_dfs = end_dfs - start_dfs;
    // std::cout << "DFS Time taken : " << elapsed_dfs.count() << " seconds" << std::endl;
    // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
    return placement_cand;
}

Pshape *placement() {
    std::vector<Transistor *> pmos_group1;
    std::vector<Transistor *> ungrouped_pmos;
    std::set<Transistor *> inout_pmos;

    int total_finger = 0;
    for (Transistor *tr_p : pmos) {
        total_finger = total_finger + std::max(tr_p->num_finger, tr_pairs[tr_p]->num_finger);
    }
    std::cout << "total_finger: " << total_finger << std::endl;

    if (total_finger < 12) {
        expected_row_num = 1;
    } else if (total_finger < 25) {
        expected_row_num = 2;
    } else if (total_finger < 37) {
        expected_row_num = 3;
    } else if (total_finger < 89) {
        expected_row_num = 4;
    } else {
        expected_row_num = 8;
    }
    // expected_row_num = 1;
    std::cout << "expected #row: " << expected_row_num << std::endl;

    std::cout << "outputs.size(): " << outputs.size() << std::endl;
    for (Signal *output : outputs) {
        std::cout << output->name << std::endl;
        for (Transistor *tr : pmos) {
            if (tr->drain == output || tr->source == output) {
                inout_pmos.insert(tr);
                std::cout << "output mos: " << tr->name << std::endl;
                break;
            }
        }
    }
    std::cout << "inputs.size(): " << inputs.size() << std::endl;
    for (Signal *input : inputs) {
        std::cout << input->name << std::endl;
        for (Transistor *tr : pmos) {
            if (tr->gate == input) {
                inout_pmos.insert(tr);
                std::cout << "input mos: " << tr->name << std::endl;
                break;
            }
        }
    }
    for (int i = 0; i < pmos.size(); i++) {
        ungrouped_pmos.push_back(pmos[i]);
    }
    // initialize groups
    std::vector<Group *> groups;
    // determine #rows
    std::vector<Transistor *> leader_cands;
    std::vector<Transistor *> other_cands;
    int num_cand = 0;
    for (auto tr : inout_pmos) {
        if (outputs.find(tr->drain) != outputs.end() || outputs.find(tr->gate) != outputs.end() || outputs.find(tr->source) != outputs.end()) {
            leader_cands.push_back(tr);
        } else {
            other_cands.push_back(tr);
        }
    }
    leader_cands.insert(leader_cands.end(), other_cands.begin(), other_cands.end());
    for (int i = 0; i < expected_row_num; i++) {
        Transistor *tr_o;
        if (i < leader_cands.size())
            tr_o = leader_cands[i];
        else
            tr_o = ungrouped_pmos[0];
        Group *gp = new Group();
        gp->trs.push_back(tr_o);
        gp->width = std::max(tr_o->num_finger, tr_pairs[tr_o]->num_finger);
        groups.push_back(gp);
        ungrouped_pmos.erase(std::remove(ungrouped_pmos.begin(), ungrouped_pmos.end(), tr_o), ungrouped_pmos.end());
    }
    std::cout << "[ungrouped_pmos]" << std::endl;
    for (int i = 0; i < ungrouped_pmos.size(); i++) {
        std::cout << ungrouped_pmos[i]->name << std::endl;
    }
    while (!ungrouped_pmos.empty()) {
        std::unordered_set<Transistor *> selected;  // 紀錄這一輪選到的 tr_cand，避免重複分配
        std::unordered_map<Group *, Transistor *> groupToCand;

        for (Group *group : groups) {
            int max_common = 0;
            Transistor *tr_cand = nullptr;
            std::vector<Signal *> sig_ref;

            for (auto tr_p : group->trs) {
                sig_ref.push_back(tr_p->drain);
                sig_ref.push_back(tr_p->gate);
                sig_ref.push_back(tr_p->source);
                sig_ref.push_back(tr_pairs[tr_p]->drain);
                sig_ref.push_back(tr_pairs[tr_p]->gate);
                sig_ref.push_back(tr_pairs[tr_p]->source);
            }

            for (auto ungrouped_tr : ungrouped_pmos) {
                if (selected.count(ungrouped_tr)) continue;  // 這輪已經被選走，跳過

                int common_num = 0;
                for (auto sig : sig_ref) {
                    if (sig->name != "VDD" && sig->name != "VSS") {
                        if (sig == ungrouped_tr->drain) common_num += 2;
                        if (sig == ungrouped_tr->gate) common_num += 1;
                        if (sig == ungrouped_tr->source) common_num += 2;
                        if (sig == tr_pairs[ungrouped_tr]->drain) common_num += 2;
                        if (sig == tr_pairs[ungrouped_tr]->gate) common_num += 1;
                        if (sig == tr_pairs[ungrouped_tr]->source) common_num += 2;
                    }
                }

                if (common_num > max_common || tr_cand == nullptr) {
                    max_common = common_num;
                    tr_cand = ungrouped_tr;
                }
            }

            if (tr_cand != nullptr) {
                groupToCand[group] = tr_cand;
                selected.insert(tr_cand);
            }
        }

        // 將這輪選中的 transistor 實際加入對應的 group，並從 ungrouped_pmos 移除
        for (auto &[group, tr] : groupToCand) {
            group->trs.push_back(tr);
            ungrouped_pmos.erase(std::remove(ungrouped_pmos.begin(), ungrouped_pmos.end(), tr), ungrouped_pmos.end());
        }
    }

    // group placement
    std::vector<std::vector<Pshape *>> placement_eachrow(groups.size());
    for (int i = 0; i < groups.size(); i++) {
        std::cout << "group: " << std::endl;
        for (int j = 0; j < groups[i]->trs.size(); j++) {
            std::cout << groups[i]->trs[j]->name << std::endl;
        }
        std::vector<Pshape *> row_placement = group_placement(groups[i]->trs);
        for (int j = 0; j < row_placement.size(); j++) {
            Pshape *pshape = row_placement[j];
            placement_eachrow[i].push_back(pshape);
        }
    }

    // std::cout << "[HPML sorting]" << std::endl;
    // for (int i = 0; i < placement_eachrow.size(); i++) {
    //     for (int j = 0; j < placement_eachrow[i].size(); j++) {
    //         placement_eachrow[i][j]->allign();
    //         placement_eachrow[i][j]->calculate_hpml();
    //         // std::cout << "HPML: " << placement_eachrow[i][j]->calculate_hpml() << std::endl;
    //     }
    //     std::sort(placement_eachrow[i].begin(), placement_eachrow[i].end(), [](Pshape *a, Pshape *b) { return a->hpml < b->hpml; });
    // }

    // common signals
    std::vector<std::set<Signal *>> row_placement_signals;
    for (int i = 0; i < placement_eachrow.size(); i++) {
        std::vector<Signal *> sig_array;
        Pshape *pshape = placement_eachrow[i][0];
        std::cout << "HPML: " << pshape->hpml << std::endl;
        for (Transistor *tr : pshape->multirow_tr_permutation_down[0]) {
            if (tr == nullptr) continue;
            sig_array.push_back(tr->drain);
            sig_array.push_back(tr->gate);
            sig_array.push_back(tr->source);
            sig_array.push_back(tr_pairs[tr]->drain);
            sig_array.push_back(tr_pairs[tr]->gate);
            sig_array.push_back(tr_pairs[tr]->source);
        }
        std::set<Signal *> s(sig_array.begin(), sig_array.end());
        s.erase(signals["VDD"]);
        s.erase(signals["VSS"]);
        row_placement_signals.push_back(s);
    }
    std::vector<std::set<Signal *>> common_sigs_row = findIntersectingElements(row_placement_signals);

    std::vector<Pshape *> single_row_vec;  // select best placement for each group (single-row)
    std::vector<Pshape *> pshape_vec;      // placement going to router

    // choose best single-row
    for (int i = 0; i < placement_eachrow.size(); i++) {
        std::priority_queue<Pshape *, std::vector<Pshape *>, decltype(&compareRoutability)> pq(compareRoutability);
        std::vector<Pshape *> best_results;
        for (int j = 0; j < placement_eachrow[i].size(); j++) {
            Pshape *pshape = placement_eachrow[i][j];
            pshape->hsp = hsp(pshape);
            pshape->hcd = hcd(pshape);
            pq.push(pshape);
            // choose the best routing-friendly pshape
            if (pq.size() > 1) {
                pq.pop();
            }
        }
        while (!pq.empty()) {
            best_results.push_back(pq.top());
            pq.pop();
        }
        std::cout << "Row " << i << ": " << std::endl;
        for (int n = 0; n < best_results.size(); n++) {
            Pshape *pshape = best_results[n];
            pshape->multirow_signal_permutation_up.assign(1, std::vector<Signal *>(pshape->multirow_tr_permutation_up[0].size() * 2 + 1));
            for (int j = 0; j < pshape->multirow_tr_permutation_up[0].size(); j++) {
                Transistor *tr_n = pshape->multirow_tr_permutation_up[0][j];
                if (tr_n) {
                    switch (pshape->multirow_tr_shape_up[0][j]) {
                        case 0:
                            pshape->multirow_signal_permutation_up[0][2 * j] = tr_n->drain;
                            pshape->multirow_signal_permutation_up[0][2 * j + 1] = tr_n->gate;
                            pshape->multirow_signal_permutation_up[0][2 * j + 2] = tr_n->source;
                            break;
                        case 1:
                            pshape->multirow_signal_permutation_up[0][2 * j] = tr_n->source;
                            pshape->multirow_signal_permutation_up[0][2 * j + 1] = tr_n->gate;
                            pshape->multirow_signal_permutation_up[0][2 * j + 2] = tr_n->drain;
                            break;
                        case 2:
                            break;
                    }
                }
            }

            pshape->multirow_signal_permutation_down.assign(1, std::vector<Signal *>(pshape->multirow_tr_permutation_down[0].size() * 2 + 1));

            for (int j = 0; j < pshape->multirow_tr_permutation_down[0].size(); j++) {
                Transistor *tr_p = pshape->multirow_tr_permutation_down[0][j];
                if (tr_p) {
                    switch (pshape->multirow_tr_shape_down[0][j]) {
                        case 0:
                            pshape->multirow_signal_permutation_down[0][2 * j] = tr_p->drain;
                            pshape->multirow_signal_permutation_down[0][2 * j + 1] = tr_p->gate;
                            pshape->multirow_signal_permutation_down[0][2 * j + 2] = tr_p->source;
                            break;
                        case 1:
                            pshape->multirow_signal_permutation_down[0][2 * j] = tr_p->source;
                            pshape->multirow_signal_permutation_down[0][2 * j + 1] = tr_p->gate;
                            pshape->multirow_signal_permutation_down[0][2 * j + 2] = tr_p->drain;
                            break;
                        case 2:
                            break;
                    }
                }
            }

            pshape->print_pshape();
            std::cout << "HSP: " << pshape->hsp << std::endl;
            std::cout << "HCD: " << pshape->hcd << std::endl;
        }
        // best pshape in each row
        single_row_vec.push_back(best_results[0]);
    }

    // single-row placement abutting
    // Pshape *multirow_pshape = placement_abutting(single_row_vec);
    Pshape *multirow_pshape = placement_merging(single_row_vec);
    multirow_pshape->print_pshape();

    multirow_pshape->allign();
    return multirow_pshape;
}

std::vector<Pshape *> group_placement(std::vector<Transistor *> pmos_group) {
    std::cout << "hello group_placement" << std::endl;
    std::vector<Pshape *> final_placement_cand;
    double bfs_total_time = 0;
    double dfs_total_time = 0;
    tr_size_sum = 0;
    int min_cell_area = std::numeric_limits<int>::max() - 1;
    int min_macro_area = std::numeric_limits<int>::max() - 1;

    for (Transistor *tr : pmos_group) {
        tr_size_sum = tr_size_sum + std::max(tr->num_finger, tr_pairs[tr]->num_finger);
    }

    int target_area = tr_size_sum;

    // assign shape to each transistor
    for (Transistor *tr : pmos_group) {
        std::vector<Lambda *> multi_row_config;
        for (auto _p_shape : phi_merged[tr]) {
            for (auto lamb : _p_shape.second) {
                multi_row_config.push_back(lamb);
            }
        }
        multi_row_configs.insert(std::make_pair(tr, multi_row_config));
    }

    while (final_placement_cand.size() == 0) {
        double max_dfs_time_taken = 0;
        auto start_while = std::chrono::high_resolution_clock::now();
        auto start_bfs = std::chrono::high_resolution_clock::now();
        auto item = bfs_placement(target_area, pmos_group);
        auto end_bfs = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_bfs = end_bfs - start_bfs;
        std::cout << "BFS Time taken : " << elapsed_bfs.count() << std::endl;
        bfs_total_time += elapsed_bfs.count();
        if (item.second.size() != 0) {
            final_placement_cand = item.second;
            break;
        } else {
            auto node_to_be_processed = item.first;
            auto start_dfs_total = std::chrono::high_resolution_clock::now();
#pragma omp parallel for schedule(dynamic)
            for (auto n : node_to_be_processed) {
                if (final_placement_cand.size() >= max_placement_size) continue;
                auto start_dfs = std::chrono::high_resolution_clock::now();
                int pshape_size = n->partial_shapes.size();
                auto new_placement_cand = dfs_placement(n, target_area, pmos_group);
                auto end_dfs = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed_dfs = end_dfs - start_dfs;
                // std::cout << "DFS Time taken : " << elapsed_dfs.count() << " seconds #pshape: " << pshape_size << std::endl;
                max_dfs_time_taken = std::max(max_dfs_time_taken, elapsed_dfs.count());
                for (auto pshape : new_placement_cand) {
                    final_placement_cand.push_back(pshape);
                }
                int thread_id = omp_get_thread_num();
            }
            auto end_dfs_total = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed_dfs_total = end_dfs_total - start_dfs_total;
            dfs_total_time += elapsed_dfs_total.count();
        }
        target_area++;
        auto end_while = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_while = end_while - start_while;
        std::cout << "Time taken : " << elapsed_while.count() << " seconds" << std::endl;
    }

    std::cout << "bfs total time: " << bfs_total_time << std::endl;
    std::cout << "dfs total time: " << dfs_total_time << std::endl;

    std::cout << "number of cell: " << final_placement_cand.size() << std::endl;
    int place_cand_id = 0;

    return final_placement_cand;
}
