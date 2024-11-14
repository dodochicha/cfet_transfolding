#include <cassert>
#include <iostream>
#include <sstream>
#include <iostream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <limits> 
#include <iomanip>
#include "z3++.h"
#include "cfet.h"

using namespace z3;

void CFET::placement_single_row() {
    context c;
    
    optimize opt(c);

    solver s(c);

    std::vector<expr> tr_pos_x;
    std::vector<expr> tr_pos_y;

    for (Transistor* tr: pmos) {
        // assign single row shape
        for (Shape* shape: tr->es) {
            if (shape->name[2] == '1') {
                tr->single_row_shape = shape;
            }
        }
        std::string key = tr->single_row_shape->name + "_" + tr->name;
        std::cout << tr->name << std::endl;
        std::vector<std::vector<std::vector<int>>> single_row_config;
        // for (const auto& pair : phi[key]) {
        //     if (pair.first[0] == 'n') {
        //         std::cout << pair.first << std::endl;
        //         for (int i = 0 ; i < pair.second.size() ; i++) {
        //             for (int j = 0 ; j < pair.second[0].size() ; j++) {
        //                 std::cout << pair.second[i][j];
        //             }
        //             std::cout << std::endl;
        //         }
        //         single_row_config.push_back(pair.second);
        //     }
        // }
        for (auto _p_shape: phi[tr]) {
            for (auto lamb: _p_shape.second) {
                if (lamb->name == "single-row") {
                    single_row_config.push_back(lamb->config);
                }
            }
        }
        single_row_configs.insert(std::make_pair(tr, single_row_config));
    }

    // declare vars
    std::cout << "declare vars" << std::endl;
    for (int i = 0 ; i < pmos.size() ; i++) {
        std::stringstream x_name;
        std::stringstream y_name;
        x_name << pmos[i]->name << "_x";
        y_name << pmos[i]->name << "_y";
        tr_pos_x.push_back(c.int_const(x_name.str().c_str()));
        tr_pos_y.push_back(c.int_const(y_name.str().c_str()));
    }
    for (int i = 0 ; i < tr_pos_y.size() ; i++) {
        opt.add(tr_pos_x[i] >= 0);
        opt.add(tr_pos_y[i] == 0);
    }
    // rpc
    std::cout << "rpc" << std::endl;
    std::vector<std::vector<expr>> ds_bools;
    ds_bools.assign(pmos.size(), std::vector<expr>());
    for (int i = 0 ; i < pmos.size() ; i++) {
        ds_bools[i].assign(pmos.size(), c.bool_const("bool"));
    }
    std::vector<std::vector<expr>> shape_idx;
    shape_idx.assign(pmos.size(), std::vector<expr>());
    for (int i = 0 ; i < pmos.size() ; i++) {
        shape_idx[i].assign(pmos.size(), c.int_const("int"));
    }
    std::vector<std::vector<expr>> ds;
    for (int i = 0 ; i < pmos.size() ; i++) {
        for (int j = i + 1 ; j < pmos.size() ; j++) {

                std::stringstream shape_ji;
                std::stringstream shape_ij;
                shape_ji << pmos[j]->name << "_" << pmos[i]->name << "_shape_idx";
                shape_ij << pmos[i]->name << "_" << pmos[j]->name << "_shape_idx";
                expr shape_ji_idx = c.int_const(shape_ji.str().c_str());
                expr shape_ij_idx = c.int_const(shape_ij.str().c_str());
                shape_idx[j][i] = shape_ji_idx;
                shape_idx[i][j] = shape_ij_idx;
                int shape_ji_size = single_row_configs[pmos[i]].size() * single_row_configs[pmos[j]].size();
                int shape_ij_size = single_row_configs[pmos[i]].size() * single_row_configs[pmos[j]].size();
                std::vector<expr> ds_jis;
                std::vector<expr> ds_ijs;
                opt.add(shape_ji_idx >= 0);
                opt.add(shape_ij_idx >= 0);
                opt.add(shape_ji_idx < shape_ji_size);
                opt.add(shape_ij_idx < shape_ij_size);
                for (int i_config_idx = 0 ; i_config_idx < single_row_configs[pmos[i]].size() ; i_config_idx++) {
                    for (int j_config_idx = 0 ; j_config_idx < single_row_configs[pmos[j]].size() ; j_config_idx++) {
                        std::stringstream ds_ji;
                        std::stringstream ds_ij;
                        ds_ji << pmos[j]->name << "_" << pmos[i]->name << "_" << i_config_idx << "_" << j_config_idx;
                        ds_ij << pmos[i]->name << "_" << pmos[j]->name << "_" << j_config_idx << "_" << i_config_idx;
                        expr trj_tri = c.bool_const(ds_ji.str().c_str()); // (j, i)
                        expr tri_trj = c.bool_const(ds_ij.str().c_str()); // (i, j)
                        ds_jis.push_back(trj_tri);
                        ds_ijs.push_back(tri_trj);
                        bool ds_ji_bool = diffusion_sharing_single_row(
                            single_row_configs[pmos[j]][j_config_idx], 
                            single_row_configs[pmos[i]][i_config_idx],
                            pmos[j],
                            pmos[i]);
                        bool ds_ij_bool = diffusion_sharing_single_row(
                            single_row_configs[pmos[i]][i_config_idx], 
                            single_row_configs[pmos[j]][j_config_idx],
                            pmos[i],
                            pmos[j]);
                        if (ds_ji_bool) opt.add(trj_tri);
                        else opt.add(!trj_tri);
                        if (ds_ij_bool) opt.add(tri_trj);
                        else opt.add(!tri_trj);
                    }
                }
                std::stringstream ds_ji_name;
                std::stringstream ds_ij_name;
                ds_ji_name << "ds_" << pmos[j]->name << "_" << pmos[i]->name;
                ds_ij_name << "ds_" << pmos[i]->name << "_" << pmos[j]->name;
                expr ds_ji_bool = c.bool_const(ds_ji_name.str().c_str());
                expr ds_ij_bool = c.bool_const(ds_ij_name.str().c_str());
                ds_bools[j][i] = ds_ji_bool;
                ds_bools[i][j] = ds_ij_bool;
                for (int k = 0 ; k < shape_ji_size ; k++) {
                    opt.add(implies(shape_ji_idx == k, ds_ji_bool == ds_jis[k]));
                    opt.add(implies(shape_ij_idx == k, ds_ij_bool == ds_ijs[k]));
                }   
                ds.push_back(ds_jis);
                ds.push_back(ds_ijs);

        }
    }

    // permutation
    std::vector<expr> tr_idxes;
    expr_vector t(c);
    for (int i = 0 ; i < pmos.size() ; i++) {
        std::stringstream tr_idx_name;
        tr_idx_name << pmos[i]->name << "_idx";
        expr tr_idx = c.int_const(tr_idx_name.str().c_str());
        int pmos_size = pmos.size();
        tr_idxes.push_back(tr_idx);
        opt.add(tr_idx >= 0);
        opt.add(tr_idx < pmos_size);
        t.push_back(tr_idx);
    }
    opt.add(distinct(t));
    for (int i = 0 ; i < pmos.size() ; i++) {
        for (int j = i + 1 ; j < pmos.size() ; j++) {
            opt.add(implies(tr_idxes[i] == tr_idxes[j] + 1, 
                ite(ds_bools[j][i], tr_pos_x[i] == tr_pos_x[j] + std::max(pmos[j]->num_finger, tr_pairs[pmos[j]]->num_finger), tr_pos_x[i] == tr_pos_x[j] + std::max(pmos[j]->num_finger, tr_pairs[pmos[j]]->num_finger) + diffusion_break_constraint)));
            opt.add(implies(tr_idxes[i] + 1 == tr_idxes[j], 
                ite(ds_bools[i][j], tr_pos_x[i] + std::max(pmos[i]->num_finger, tr_pairs[pmos[i]]->num_finger) == tr_pos_x[j], tr_pos_x[i] + std::max(pmos[i]->num_finger, tr_pairs[pmos[i]]->num_finger) + diffusion_break_constraint == tr_pos_x[j])));
        }
    }
    
    // minimize area
    expr min_width = c.int_const("min_width");
    for (int i = 0 ; i < pmos.size() ; i++) {
        opt.add(min_width >= tr_pos_x[i] + std::max(pmos[i]->num_finger, tr_pairs[pmos[i]]->num_finger));
    }
    optimize::handle h1 = opt.minimize(min_width);

    // determine shape
    std::vector<expr> tr_shape_idxes;
    for (int i = 0 ; i < pmos.size(); i++) {
        std::stringstream tr_shape_idx_name;
        tr_shape_idx_name << pmos[i]->name << "_tr_shape_idx";
        expr tr_shape_idx = c.int_const(tr_shape_idx_name.str().c_str());
        opt.add(tr_shape_idx >= 0);
        int tr_shape_size = single_row_configs[pmos[i]].size();
        opt.add(tr_shape_idx < tr_shape_size);
        tr_shape_idxes.push_back(tr_shape_idx);
    }
    for (int i = 0 ; i < pmos.size(); i++) {
        for (int j = i + 1 ; j < pmos.size(); j++) {
            int config_size = single_row_configs[pmos[i]].size();
            opt.add(shape_idx[j][i] == config_size * tr_shape_idxes[i] + tr_shape_idxes[j]);
            opt.add(shape_idx[i][j] == config_size * tr_shape_idxes[i] + tr_shape_idxes[j]);
        }
    }

    // opt.add(min_width == 16);

    // check SAT
    if (opt.check() == sat) {
        std::cout << "SAT\n";
        model m = opt.get_model();
        std::cout << "min_width: " << opt.lower(h1) << std::endl;
        width_layout = opt.lower(h1).get_numeral_int();
        tr_layout.assign(pmos.size(), 0);
        
        for (int i = 0 ; i < pmos.size() ; i++) {
            std::cout << pmos[i]->name << " " << m.eval(tr_pos_x[i]) << std::endl;
        }
        for (int i = 0 ; i < tr_idxes.size() ; i++) {
            int idx = m.eval(tr_idxes[i]).get_numeral_int();
            tr_layout[idx] = i;
            // std::cout << tr_layout[m.eval(tr_idxes[i])]->name << std::endl;
        }
        for (int i = 0 ; i < tr_idxes.size() ; i++) {
            std::cout << pmos[tr_layout[i]]->name << std::endl;
        }
        for (int i = 0 ; i < ds_bools.size() ; i++) {
            ds_layout.push_back(std::vector<bool>());
            for (int j = 0 ; j < ds_bools.size() ; j++) {
                std::cout << ds_bools[i][j] << ": " << m.eval(ds_bools[i][j]).is_true() << std::endl;
                ds_layout[i].push_back(m.eval(ds_bools[i][j]).is_true());
            }   
        }
        for (int i = 0 ; i < shape_idx.size() ; i++) {
            for (int j = 0 ; j < shape_idx[i].size() ; j++) {
                std::cout << shape_idx[i][j] << ": " << m.eval(shape_idx[i][j]) << std::endl;
            }
        }
        for (int i = 0 ; i < tr_shape_idxes.size() ; i++) {
            tr_shape_idx_layout.push_back(m.eval(tr_shape_idxes[i]).get_numeral_int());
        }
        for (int i = 0 ; i < tr_shape_idxes.size() ; i++) {
            std::cout << pmos[i]->name << " " << m.eval(tr_shape_idxes[i]).get_numeral_int() << std::endl;
        }
        for (int i = 0 ; i < ds.size() ; i++) {
            for (int j = 0 ; j < ds[i].size() ; j++) {
                std::cout << ds[i][j] << ": " << m.eval(ds[i][j]) << std::endl;
            }
            std::cout << std::endl;
        }
        std::cout << "finish placing" << std::endl;
    } else {
        std::cout << "UNSAT\n";
    }
    for (int i = 0 ; i < pmos.size() ; i++) {
        std::cout << "num_finger: " << std::max(pmos[i]->num_finger, tr_pairs[pmos[i]]->num_finger) << std::endl;
    }
}

void CFET::placement_multi_row_search_tree_bottom_up() {
    for (Transistor* p: pmos) {
        Transistor* n = tr_pairs[p];
        p->num_finger = (p->width-0.1) / max_cfet_width + 1;
        // std::cout << p->name << " num_finger: " << p->num_finger << std::endl;
        // std::cout << p->drain->name << " " << p->gate->name << " " << p->source->name << std::endl;
        n->num_finger = (n->width-0.1) / max_cfet_width + 1;
        // std::cout << n->name << " num_finger: " << n->num_finger << std::endl;
        // std::cout << n->drain->name << " " << n->gate->name << " " << n->source->name << std::endl;
    }
    // for (Transistor* n: nmos) {
    //     n->num_finger = (n->width-0.1) / max_cfet_width + 1;
    //     std::cout << n->name << " num_finger: " << n->num_finger << std::endl;
    //     std::cout << n->drain->name << " " << n->gate->name << " " << n->source->name << std::endl;
    // }
    std::vector<Pshape*> placement_cand;
    int tr_size_sum = 0;
    int min_cell_area = std::numeric_limits<int>::max() - 1;
    int min_macro_area = std::numeric_limits<int>::max() - 1;

    for (Transistor* tr: pmos) {
        tr_size_sum = tr_size_sum + std::max(tr->num_finger, tr_pairs[tr]->num_finger);
    }
    
        for (Transistor* tr: pmos) {
            std::vector<Lambda*> multi_row_config;
            for (auto _p_shape: phi_merged[tr]) {
                for (auto lamb: _p_shape.second) {
                    multi_row_config.push_back(lamb);
                }
            }
            multi_row_configs.insert(std::make_pair(tr, multi_row_config));
        }
    

    // int target_area = tr_size_sum + 2;
    int target_area = tr_size_sum;
    while (placement_cand.size() == 0) {
        std::cout << "target_area: " << target_area << std::endl;
        Node* root = new Node();
        for (int i = 0; i < pmos.size(); i++) {
            // create node
            bool pruned = false;
            std::cout << "i: " << i << std::endl;
            Node* current_node = new Node();
            std::vector<Transistor*> remained_pmos(pmos);
            Transistor* current_tr = pmos[i];
            std::vector<Pshape* > partial_placement;
            int tr_left_sum = tr_size_sum;
            current_node->parent = root;
            remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), pmos[i]), remained_pmos.end());
            current_node->remained_pmos = remained_pmos;
            current_node->tr = current_tr;
            current_node->fill = 0;
            tr_left_sum = tr_left_sum - std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);

            // create partial shape
            for (auto lamb: multi_row_configs[current_tr]) {
                Pshape* pshape = new Pshape();
                // multi-row
                pshape->multirow_area = std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                pshape->multirow_tr_shape_up = lamb->config_up;
                pshape->multirow_tr_shape_down = lamb->config_down;
                pshape->multirow_tr_permutation_up.assign(lamb->config_up.size(), std::vector<Transistor*>(lamb->config_up[0].size()));
                pshape->multirow_tr_permutation_down.assign(lamb->config_down.size(), std::vector<Transistor*>(lamb->config_down[0].size()));
                pshape->height = 0;
                pshape->width = 0;
                pshape->top_width = 0;
                for (int row = 0; row < lamb->config_up.size(); row++) {
                    pshape->most_right_idx.push_back(0);
                    for (int col  = 0; col < lamb->config_up[row].size(); col++) {
                        if (lamb->config_up[row][col] != 2) {
                            pshape->multirow_tr_permutation_up[row][col] = tr_pairs[current_tr];
                            pshape->most_right_idx[row] = col;
                            pshape->height = row + 1;
                        }
                        else {
                            pshape->multirow_tr_permutation_up[row][col] = nullptr;
                        }
                        if (lamb->config_down[row][col] != 2) {
                            pshape->multirow_tr_permutation_down[row][col] = current_tr;
                            pshape->most_right_idx[row] = col;
                            pshape->height = row + 1;
                        }
                        else {
                            pshape->multirow_tr_permutation_down[row][col] = nullptr;
                        }
                        if (lamb->config_up[row][col] != 2 || lamb->config_down[row][col] != 2) {
                            if (col + 1 > pshape->width) {
                                pshape->width = col + 1;
                            }
                        }
                    }
                    if (row == lamb->config_up.size() - 1) {
                        for (int col  = 0; col < lamb->config_up[row].size(); col++) {
                            if (lamb->config_up[row][col] != 2 || lamb->config_down[row][col] != 2) {
                                pshape->top_width = col + 1;
                            }
                        }
                    }
                }
                // std::cout << "pshape->width: " << pshape->width << std::endl;
                pshape->multirow_macro_area = (pshape->width + 2) * pshape->height;
                // std::cout << pshape->multirow_tr_permutation_down[0][0]->name << " " << pshape->multirow_tr_permutation_up[0][0]->name << std::endl;
                partial_placement.push_back(pshape);
            }
            // assign partial_placement
            current_node->partial_shapes = partial_placement;

            // std::cout << "hello while" << std::endl;
            while (true) {
                // std::cout << "current_node->tr->name: " << current_node->tr->name << std::endl;
                if (pruned) {
                    // std::cout << "pruned!" << std::endl;
                    pruned = false;
                    while (true) {
                        Node* n = current_node->parent;
                        tr_left_sum = tr_left_sum + std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                        for (auto pshape: current_node->partial_shapes) {
                            delete pshape;
                        }
                        delete current_node;
                        current_node = n;
                        current_tr = n->tr;
                        if (((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) == false) {
                            break;
                        }
                    }
                    current_node->fill++;
                    if (current_node == root) {
                        break;
                    }
                }
                else if (current_node->remained_pmos.size() == 0) {
                    // std::cout << "leaf!" << std::endl;
                    if (current_node->partial_shapes.size() != 0) std::cout << "current_node->partial_shapes.size(): " << current_node->partial_shapes.size() << " area: " << current_node->partial_shapes[0]->multirow_macro_area << std::endl;
                    if (current_node->partial_shapes.size() != 0) {
                        // std::cout << current_node->partial_shapes[0]->multirow_area << " " << min_cell_area << std::endl;
                        if (current_node->partial_shapes[0]->multirow_area < min_cell_area) {
                            min_cell_area = current_node->partial_shapes[0]->multirow_area;
                            std::cout << "min_cell_area: " << min_cell_area << std::endl;
                            for (auto item: placement_cand) {
                                delete item;
                                item = nullptr;
                            }
                            placement_cand.clear();
                            placement_cand.shrink_to_fit();
                            for (auto item: current_node->partial_shapes) {
                                if (item->multirow_macro_area < min_macro_area) {
                                    for (auto cand: placement_cand) {
                                        delete cand;
                                        cand = nullptr;
                                    }
                                    placement_cand.clear();
                                    placement_cand.shrink_to_fit();
                                    min_macro_area = item->multirow_macro_area;
                                    placement_cand.push_back(item);
                                }
                                else if (item->multirow_macro_area == min_macro_area) {
                                    placement_cand.push_back(item);
                                }
                                else {
                                    delete item;
                                    item = nullptr;
                                }
                            }
                            // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
                            // std::cout << "min_cell_area: " << min_cell_area << std::endl;
                        }
                        else if (current_node->partial_shapes[0]->multirow_area == min_cell_area) {
                            for (auto item: current_node->partial_shapes) {
                                if (placement_cand.size() < max_placement_size) {
                                    // std::cout << "item->min_macro_area: " << item->multirow_macro_area << std::endl;
                                    if (item->multirow_macro_area < min_macro_area) {
                                        for (auto cand: placement_cand) {
                                            delete cand;
                                            cand = nullptr;
                                        }
                                        min_macro_area = item->multirow_macro_area;
                                        placement_cand.clear();
                                        placement_cand.shrink_to_fit();
                                        placement_cand.push_back(item);
                                    }
                                    else if (item->multirow_macro_area == min_macro_area) {
                                        placement_cand.push_back(item);
                                    }
                                    else {
                                        delete item;
                                        item = nullptr;
                                    }
                                }
                                else {
                                    delete item;
                                    item = nullptr;
                                    break;
                                }
                            }
                            if (current_node->partial_shapes.size() == 0) {
                                delete current_node;
                            }
                            if (placement_cand.size() >= max_placement_size) {
                                break;
                            }
                        }
                        else {
                            // std::cout << "else " << std::endl;
                        }
                        // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
                    }
                    else {
                        delete current_node;
                    }
                    while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
                        Node* n = current_node->parent;
                        tr_left_sum = tr_left_sum + std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                        current_node = n;
                        current_tr = n->tr;
                    }
                    current_node->fill++;
                    if (current_node == root) {
                        break;
                    }
                }
                else {
                    int min_partial_area = std::numeric_limits<int>::max();
                    if (current_node->partial_shapes.size() == 0) {
                        current_node->partial_shapes.clear();
                        current_node->partial_shapes.shrink_to_fit();
                        pruned = true;
                        continue;
                    }
                    Node* n = new Node();
                    std::vector<Transistor*> remained_pmos = current_node->remained_pmos;
                    Transistor* old_tr = current_tr;
                    current_tr = remained_pmos[current_node->fill];
                    // std::cout << "now current_tr: " << current_tr->name << std::endl;
                    std::vector<Pshape*> new_partial_placement;
                    int low_bound;
                    // std::cout << "tr_left_sum before: " << tr_left_sum << std::endl;
                    tr_left_sum = tr_left_sum - std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                    // std::cout << "tr_left_sum: " << tr_left_sum << std::endl;
                    for (auto old_pshape: current_node->partial_shapes) {
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
                        for (auto lamb: multi_row_configs[current_tr]) {
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
                            // for (int row = 0; row <= old_pshape->height; row++) {
                            // std::cout << "test: " << old_pshape->height - 1 << " " << old_pshape->height << std::endl;
                            for (int row = std::max(0, static_cast<int>(old_pshape->height - lamb->config_up.size())); row <= old_pshape->height; row++) {
                                int col;
                                if (row < old_pshape->multirow_tr_shape_up.size()) {
                                    col = old_pshape->most_right_idx[row];
                                }
                                else {
                                    col = 0;
                                }
                                while (true) {
                                    if (merge_enable(old_pshape, lamb, tr_pairs[current_tr], current_tr, row, col)) {
                                        // std::cout << "row/col: " << row << " " << col << std::endl;
                                        Pshape* pshape = merge(old_pshape, lamb, tr_pairs[current_tr], current_tr, row, col);
                                        // cut by low_bound
                                        // low_bound = pshape->multirow_area + tr_left_sum;
                                        low_bound = std::max(pshape->top_width + tr_left_sum, pshape->width) * pshape->height;
                                        // low_bound = std::max(pshape->top_width + 2 + tr_left_sum, pshape->width + 2) * pshape->height;
                                        
                                        // std::cout << "low_bound: " << low_bound << std::endl;
                                        // std::cout << "min_cell_area: " << min_cell_area << std::endl;
                                        // std::cout << pshape->height << "/ " << max_allowable_cell_height << std::endl;
                                        // std::cout << low_bound << " " << min_cell_area + 1 << std::endl;
                                        // if (low_bound > min_cell_area) {
                                        if (low_bound > target_area) {
                                            delete pshape;
                                            pshape = nullptr;
                                            break;
                                        }
                                        if (pshape->height > max_allowable_cell_height) {
                                            delete pshape;
                                            pshape = nullptr;
                                            break;
                                        }
                                        // std::cout << "merge_enable" << std::endl;
                                        // std::cout << "multi-row area: " << pshape->multirow_area << std::endl;
                                        // std::cout << "min_partial_area: " << min_partial_area << std::endl;
                                        // else if (min_partial_width < pshape->multirow_area) {
                                        //     delete pshape;
                                        //     pshape = nullptr;
                                        //     continue;
                                        // }
                                        // new_partial_placement.push_back(pshape);
                                        if (pshape->multirow_area < min_partial_area) {
                                            for (auto item: new_partial_placement) {
                                                delete item;
                                                item = nullptr;
                                            }
                                            new_partial_placement.clear();
                                            new_partial_placement.shrink_to_fit();
                                            min_partial_area = pshape->multirow_area;
                                            new_partial_placement.push_back(pshape);
                                        }
                                        else if (pshape->multirow_area == min_partial_area) {
                                            new_partial_placement.push_back(pshape);
                                        }
                                        else {
                                            delete pshape;
                                            pshape = nullptr;
                                        }
                                        break;
                                    }
                                    col++;
                                }
                            }
                        }
                    }
                    // std::cout << "min_partial_area: " << min_partial_area << std::endl;
                    // std::cout << "new_partial_placement.size(): " << new_partial_placement.size() << " min_partial_area: " << min_partial_area << std::endl;
                    n->partial_shapes = new_partial_placement;
                    remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), current_tr), remained_pmos.end());
                    n->remained_pmos = remained_pmos;
                    n->parent = current_node;
                    n->tr = current_tr;
                    n->fill = 0;
                    current_node->children.push_back(n);
                    current_node = n;
                    // std::cout << "current_node->tr->name: " << current_node->tr->name << std::endl;
                    // break;
                }
            }
            std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
            std::cout << "min_cell_area: " << min_cell_area << std::endl;
            int _min_macro_area = std::numeric_limits<int>::max();
            for (auto _pshape: placement_cand) {
                if (_pshape->multirow_macro_area < _min_macro_area) {
                    _min_macro_area = _pshape->multirow_macro_area;
                }
            }
            std::cout << "min_macro_area: " << _min_macro_area << std::endl;
            if (placement_cand.size() >= max_placement_size) {
                break;
            }
        }
        target_area++;
    }

    // print solution
    std::cout << "min_cell_area: " << min_cell_area << std::endl;

    std::vector<Pshape*> new_placement_cand;
    for (auto place_cand: placement_cand) {
        if (place_cand->multirow_macro_area < min_macro_area) {
            min_macro_area = place_cand->multirow_macro_area;
        }
    }
    std::cout << "min_macro_area * (1 + relaxation_parameter): " << min_macro_area * (1 + relaxation_parameter) << std::endl;
    for (auto place_cand: placement_cand) {
        if (place_cand->multirow_macro_area <= min_macro_area * (1 + relaxation_parameter)) {
            new_placement_cand.push_back(place_cand);
        }
    }
    placement_cand = new_placement_cand;
    std::cout << "number of cell: " << placement_cand.size() << std::endl;
    std::cout << "min_macro_area: " << min_macro_area << std::endl;
    int place_cand_id = 0;
    for (auto pshape: placement_cand) {
        // std::cout << "min_macro_area: " << pshape->multirow_macro_area << std::endl;
        // std::cout << "min_cell_area: " << pshape->multirow_area << std::endl;
        std::cout << "#" << place_cand_id << std::endl;
        // print transistor name
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                if (pshape->multirow_tr_permutation_up[i][j] == nullptr) {
                    std::cout << "     ";
                }
                else {
                    std::cout <<  std::left << std::setw(4) << pshape->multirow_tr_permutation_up[i][j]->name << " ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << "--" << std::endl;
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                if (pshape->multirow_tr_permutation_down[i][j] == nullptr) {
                    std::cout << "     ";
                }
                else {
                    std::cout <<  std::left << std::setw(4) << pshape->multirow_tr_permutation_down[i][j]->name << " ";
                }
            }
            std::cout << std::endl;
        }
        //print active
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                Transistor* tr_n = pshape->multirow_tr_permutation_up[i][j];
                switch (pshape->multirow_tr_shape_up[i][j]) {
                    case 0:
                        std::cout << std::left << std::setw(7) << tr_n->drain->name << " " << std::left << std::setw(7) << tr_n->gate->name << " " << std::left << std::setw(7) << tr_n->source->name << " ";
                        break;
                    case 1:
                        std::cout << std::left << std::setw(7) << tr_n->source->name << " " << std::left << std::setw(7) << tr_n->gate->name << " " << std::left << std::setw(7) << tr_n->drain->name << " ";
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
                Transistor* tr_p = pshape->multirow_tr_permutation_down[i][j];
                switch (pshape->multirow_tr_shape_down[i][j]) {
                    case 0:
                        std::cout << std::left << std::setw(7) << tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
                        break;
                    case 1:
                        std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
                        break;
                    case 2:
                        std::cout << "------------------------";
                        break;
                }
            }
            std::cout << std::endl;
        }
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                std::cout << pshape->multirow_tr_shape_up[i][j];
            }
            std::cout << std::endl;
        }
        std::cout << "--" << std::endl;
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                std::cout << pshape->multirow_tr_shape_down[i][j];
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
        place_cand_id++;
    }
}

void CFET::new_placement_multi_row_search_tree() {
    // calculate num_finger
    for (Transistor* p: pmos) {
        Transistor* n = tr_pairs[p];
        p->num_finger = (p->width-0.1) / max_cfet_width + 1;
        n->num_finger = (n->width-0.1) / max_cfet_width + 1;
    }
    std::vector<Pshape*> placement_cand;
    int tr_size_sum = 0;
    int min_cell_area = std::numeric_limits<int>::max() - 1;
    int min_macro_area = std::numeric_limits<int>::max() - 1;

    for (Transistor* tr: pmos) {
        tr_size_sum = tr_size_sum + std::max(tr->num_finger, tr_pairs[tr]->num_finger);
    }
    
        for (Transistor* tr: pmos) {
            std::vector<Lambda*> multi_row_config;
            for (auto _p_shape: phi_merged[tr]) {
                for (auto lamb: _p_shape.second) {
                    multi_row_config.push_back(lamb);
                }
            }
            multi_row_configs.insert(std::make_pair(tr, multi_row_config));
        }
    
        Node* root = new Node();
        for (int i = 0; i < pmos.size(); i++) {
            // create node
            bool pruned = false;
            std::cout << "i: " << i << " current_tr: " << pmos[i]->name << std::endl;
            Node* current_node = new Node();
            std::vector<Transistor*> remained_pmos(pmos);
            Transistor* current_tr = pmos[i];
            std::vector<Pshape* > partial_placement;
            int tr_left_sum = tr_size_sum;
            current_node->parent = root;
            remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), pmos[i]), remained_pmos.end());
            current_node->remained_pmos = remained_pmos;
            current_node->tr = current_tr;
            current_node->fill = 0;
            tr_left_sum = tr_left_sum - std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
            // create partial shape
            for (auto lamb: multi_row_configs[current_tr]) {
                Pshape* pshape = new Pshape();
                // multi-row
                pshape->multirow_area = std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                pshape->multirow_tr_shape_up = lamb->config_up;
                pshape->multirow_tr_shape_down = lamb->config_down;
                pshape->multirow_tr_permutation_up.assign(lamb->config_up.size(), std::vector<Transistor*>(lamb->config_up[0].size()));
                pshape->multirow_tr_permutation_down.assign(lamb->config_down.size(), std::vector<Transistor*>(lamb->config_down[0].size()));
                pshape->height = 0;
                pshape->width = 0;
                pshape->top_width = 0;
                for (int row = 0; row < lamb->config_up.size(); row++) {
                    pshape->most_right_idx.push_back(0);
                    for (int col  = 0; col < lamb->config_up[row].size(); col++) {
                        if (lamb->config_up[row][col] != 2) {
                            pshape->multirow_tr_permutation_up[row][col] = tr_pairs[current_tr];
                            pshape->most_right_idx[row] = col;
                            pshape->height = row + 1;
                        }
                        else {
                            pshape->multirow_tr_permutation_up[row][col] = nullptr;
                        }
                        if (lamb->config_down[row][col] != 2) {
                            pshape->multirow_tr_permutation_down[row][col] = current_tr;
                            pshape->most_right_idx[row] = col;
                            pshape->height = row + 1;
                        }
                        else {
                            pshape->multirow_tr_permutation_down[row][col] = nullptr;
                        }
                        if (lamb->config_up[row][col] != 2 || lamb->config_down[row][col] != 2) {
                            if (col + 1 > pshape->width) {
                                pshape->width = col + 1;
                            }
                        }
                    }
                    if (row == lamb->config_up.size() - 1) {
                        for (int col  = 0; col < lamb->config_up[row].size(); col++) {
                            if (lamb->config_up[row][col] != 2 || lamb->config_down[row][col] != 2) {
                                pshape->top_width = col + 1;
                            }
                        }
                    }
                }
                // std::cout << "pshape->width: " << pshape->width << std::endl;
                pshape->multirow_macro_area = (pshape->width) * pshape->height;
                // std::cout << pshape->multirow_tr_permutation_down[0][0]->name << " " << pshape->multirow_tr_permutation_up[0][0]->name << std::endl;
                partial_placement.push_back(pshape);
            }
            // assign partial_placement
            current_node->partial_shapes = partial_placement;

            while (true) {
                if (pruned) {
                    // std::cout << "pruned!" << std::endl;
                    pruned = false;
                    while (true) {
                        Node* n = current_node->parent;
                        tr_left_sum = tr_left_sum + std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                        for (auto pshape: current_node->partial_shapes) {
                            delete pshape;
                        }
                        delete current_node;
                        current_node = n;
                        current_tr = n->tr;
                        if (((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) == false) {
                            break;
                        }
                    }
                    current_node->fill++;
                    if (current_node == root) {
                        break;
                    }
                }
                else if (current_node->remained_pmos.size() == 0) {
                    // std::cout << "leaf!" << std::endl;
                    if (current_node->partial_shapes.size() != 0) {
                        for (auto pshape: current_node->partial_shapes) {
                            if (pshape->multirow_macro_area < min_macro_area) {
                                min_macro_area = pshape->multirow_macro_area;
                                for (auto item: placement_cand) {
                                    delete item;
                                    item = nullptr;
                                }
                                placement_cand.clear();
                                placement_cand.shrink_to_fit();
                                placement_cand.push_back(pshape);
                            }
                            else if (pshape->multirow_macro_area == min_macro_area) {
                                placement_cand.push_back(pshape);
                            }
                            else {
                                delete pshape;
                                pshape = nullptr;
                            }
                        }
                        // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
                    }
                    else {
                        delete current_node;
                    }
                    while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
                        Node* n = current_node->parent;
                        tr_left_sum = tr_left_sum + std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                        current_node = n;
                        current_tr = n->tr;
                    }
                    current_node->fill++;
                    if (current_node == root) {
                        break;
                    }
                }
                else {
                    int min_potential_macro_area = std::numeric_limits<int>::max();
                    if (current_node->partial_shapes.size() == 0) {
                        current_node->partial_shapes.clear();
                        current_node->partial_shapes.shrink_to_fit();
                        pruned = true;
                        continue;
                    }
                    Node* n = new Node();
                    std::vector<Transistor*> remained_pmos = current_node->remained_pmos;
                    Transistor* old_tr = current_tr;
                    current_tr = remained_pmos[current_node->fill];
                    // std::cout << "now current_tr: " << current_tr->name << std::endl;
                    std::vector<Pshape*> new_partial_placement;
                    int low_bound;
                    tr_left_sum = tr_left_sum - std::max(current_tr->num_finger, tr_pairs[current_tr]->num_finger);
                    for (auto old_pshape: current_node->partial_shapes) {
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
                        for (auto lamb: multi_row_configs[current_tr]) {
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
                            // for (int row = 0; row <= old_pshape->height; row++) {
                            for (int row = std::max(0, static_cast<int>(old_pshape->height - lamb->config_up.size())); row <= old_pshape->height; row++) {
                                int col;
                                if (row < old_pshape->multirow_tr_shape_up.size()) {
                                    col = old_pshape->most_right_idx[row];
                                }
                                else {
                                    col = 0;
                                }
                                while (true) {
                                    if (merge_enable(old_pshape, lamb, tr_pairs[current_tr], current_tr, row, col)) {
                                        // std::cout << "row/col: " << row << " " << col << std::endl;
                                        Pshape* pshape = merge(old_pshape, lamb, tr_pairs[current_tr], current_tr, row, col);
                                        // cut by low_bound
                                        // low_bound = pshape->multirow_area + tr_left_sum;
                                        // low_bound = std::max(pshape->top_width + tr_left_sum, pshape->width) * pshape->height;
                                        // low_bound = std::max(pshape->top_width + 2 + tr_left_sum, pshape->width + 2) * pshape->height;
                                        low_bound = std::numeric_limits<int>::max();
                                        for (int h = pshape->height; h <= max_allowable_cell_height; h++) {
                                            int _low_bound = std::max(pshape->width, (pshape->top_width + tr_left_sum + (h - pshape->height + 1) - 1) / (h - pshape->height + 1));
                                            _low_bound = _low_bound * h;
                                            // std::cout << "_low_bound: " << _low_bound << " h: " << h << std::endl;
                                            if (_low_bound < low_bound) {
                                                low_bound = _low_bound;
                                            }
                                        }
                                        
                                        // std::cout << "low_bound: " << low_bound << " min_macro_area: " << min_macro_area << " min_potential_macro_area: " << min_potential_macro_area << std::endl;
                                        // std::cout << "min_cell_area: " << min_cell_area << std::endl;
                                        // std::cout << pshape->height << "/ " << max_allowable_cell_height << std::endl;
                                        // std::cout << low_bound << " " << min_cell_area + 1 << std::endl;
                                        if (low_bound > min_macro_area) {
                                            delete pshape;
                                            pshape = nullptr;
                                            break;
                                        }
                                        if (pshape->height > max_allowable_cell_height) {
                                            delete pshape;
                                            pshape = nullptr;
                                            break;
                                        }
                                        if (low_bound < min_potential_macro_area) {
                                            for (auto item: new_partial_placement) {
                                                delete item;
                                                item = nullptr;
                                            }
                                            new_partial_placement.clear();
                                            new_partial_placement.shrink_to_fit();
                                            min_potential_macro_area = low_bound;
                                            new_partial_placement.push_back(pshape);
                                        }
                                        else if (low_bound == min_potential_macro_area) {
                                            new_partial_placement.push_back(pshape);
                                        }
                                        else {
                                            delete pshape;
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
                    remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), current_tr), remained_pmos.end());
                    n->remained_pmos = remained_pmos;
                    n->parent = current_node;
                    n->tr = current_tr;
                    n->fill = 0;
                    current_node->children.push_back(n);
                    current_node = n;
                }
            }
            std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
            // std::cout << "min_cell_area: " << min_cell_area << std::endl;
            int _min_macro_area = std::numeric_limits<int>::max();
            for (auto _pshape: placement_cand) {
                if (_pshape->multirow_macro_area < _min_macro_area) {
                    _min_macro_area = _pshape->multirow_macro_area;
                }
            }
            std::cout << "min_macro_area: " << _min_macro_area << std::endl;
            if (placement_cand.size() >= max_placement_size) {
                break;
            }
        }

    // print solution
    // std::cout << "min_cell_area: " << min_cell_area << std::endl;

    std::vector<Pshape*> new_placement_cand;
    for (auto place_cand: placement_cand) {
        if (place_cand->multirow_macro_area < min_macro_area) {
            min_macro_area = place_cand->multirow_macro_area;
        }
    }
    std::cout << "min_macro_area * (1 + relaxation_parameter): " << min_macro_area * (1 + relaxation_parameter) << std::endl;
    for (auto place_cand: placement_cand) {
        if (place_cand->multirow_macro_area <= min_macro_area * (1 + relaxation_parameter)) {
            new_placement_cand.push_back(place_cand);
        }
    }
    // placement_cand = new_placement_cand;
    std::cout << "number of cell: " << placement_cand.size() << std::endl;
    std::cout << "min_macro_area: " << min_macro_area << std::endl;
    int place_cand_id = 0;
    for (auto pshape: placement_cand) {
        // std::cout << "min_macro_area: " << pshape->multirow_macro_area << std::endl;
        // std::cout << "min_cell_area: " << pshape->multirow_area << std::endl;
        std::cout << "#" << place_cand_id << std::endl;
        // print transistor name
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                if (pshape->multirow_tr_permutation_up[i][j] == nullptr) {
                    std::cout << "     ";
                }
                else {
                    std::cout <<  std::left << std::setw(4) << pshape->multirow_tr_permutation_up[i][j]->name << " ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << "--" << std::endl;
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                if (pshape->multirow_tr_permutation_down[i][j] == nullptr) {
                    std::cout << "     ";
                }
                else {
                    std::cout <<  std::left << std::setw(4) << pshape->multirow_tr_permutation_down[i][j]->name << " ";
                }
            }
            std::cout << std::endl;
        }
        //print active
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                Transistor* tr_n = pshape->multirow_tr_permutation_up[i][j];
                switch (pshape->multirow_tr_shape_up[i][j]) {
                    case 0:
                        std::cout << std::left << std::setw(7) << tr_n->drain->name << " " << std::left << std::setw(7) << tr_n->gate->name << " " << std::left << std::setw(7) << tr_n->source->name << " ";
                        break;
                    case 1:
                        std::cout << std::left << std::setw(7) << tr_n->source->name << " " << std::left << std::setw(7) << tr_n->gate->name << " " << std::left << std::setw(7) << tr_n->drain->name << " ";
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
                Transistor* tr_p = pshape->multirow_tr_permutation_down[i][j];
                switch (pshape->multirow_tr_shape_down[i][j]) {
                    case 0:
                        std::cout << std::left << std::setw(7) << tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
                        break;
                    case 1:
                        std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
                        break;
                    case 2:
                        std::cout << "------------------------";
                        break;
                }
            }
            std::cout << std::endl;
        }
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                std::cout << pshape->multirow_tr_shape_up[i][j];
            }
            std::cout << std::endl;
        }
        std::cout << "--" << std::endl;
        for (int i = 0; i < pshape->height; i++) {
            for (int j = 0; j < pshape->width; j++) {
                std::cout << pshape->multirow_tr_shape_down[i][j];
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
        place_cand_id++;
    }
}