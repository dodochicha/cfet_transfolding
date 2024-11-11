#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <iomanip>
#include "z3++.h"
#include "cfet.h"

using namespace z3;

void CFET::test_ds() {
    for (Transistor* tr: pmos) {
        // assign single row shape
        for (Shape* shape: tr->es) {
            if (shape->name[2] == '1') {
                tr->single_row_shape = shape;
            }
        }
        std::string key = tr->single_row_shape->name + "_" + tr->name;
        // std::cout << tr->name << " " << key << std::endl;
        std::vector<std::vector<std::vector<int>>> single_row_config;
        for (const auto& pair : phi[key]) {
            if (pair.first[0] == 'n') {
                // std::cout << pair.first << std::endl;
                for (int i = 0 ; i < pair.second.size() ; i++) {
                    for (int j = 0 ; j < pair.second[0].size() ; j++) {
                        // std::cout << pair.second[i][j];
                    }
                    // std::cout << std::endl;
                }
                single_row_config.push_back(pair.second);
            }
        }
        single_row_configs.insert(std::make_pair(tr, single_row_config));
    }
    for (int i = 0; i < pmos.size(); i++) {
        for (int j = 0; j < pmos.size(); j++) {
            Transistor* tr_left = pmos[i];
            Transistor* tr_right = pmos[j];
            if (tr_left != tr_right) {
                for (int m = 0; m < single_row_configs[tr_left].size(); m++) {
                    for (int n = 0; n < single_row_configs[tr_right].size(); n++) {
                        bool ds = diffusion_sharing_single_row(
                            single_row_configs[tr_left][m], 
                            single_row_configs[tr_right][n],
                            tr_left,
                            tr_right);
                        std::cout << tr_left->name << " " << tr_right->name << " " << m << " " << n << " " << ds << std::endl;
                    }
                }
            }
        }
    }
}
\
// void CFET::test() {
//     std::unordered_map<int, Transistor*> tr_id;
//     std::vector<int> nums;
//     std::vector<Pshape*> placement_cand;
//     int tr_idx = 0;
//     int tr_size_sum = 0;
//     int min_cell_width = std::numeric_limits<int>::max();

//     for (Transistor* tr: pmos) {
//         tr_size_sum = tr_size_sum + tr->num_finger;
//     }

//     Node* root = new Node();
//     for (int i = 0; i < pmos.size(); i++) {
//         bool pruned = false;
//         std::cout << "i: " << i << std::endl;
//         Node* current_node = new Node();
//         std::vector<Transistor*> remained_pmos(pmos);
//         Transistor* current_tr = pmos[i];
//         std::vector<Pshape* > partial_placement;
//         int tr_left_sum = tr_size_sum;
//         int config_idx = 0;
//         current_node->parent = root;
//         remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), pmos[i]), remained_pmos.end());
//         current_node->remained_pmos = remained_pmos;
//         current_node->tr = current_tr;
//         current_node->fill = 0;
        
//         tr_left_sum = tr_left_sum - current_tr->num_finger;
//         // std::cout << "tr_left_sum: " << tr_left_sum << std::endl;

//         for (int config_idx = 0; config_idx <= 1; config_idx++) {
//             Pshape* pshape = new Pshape();
//             pshape->tr_permutaton.push_back(current_tr);
//             pshape->tr_shape_id.push_back(config_idx);
//             pshape->width = pmos[i]->num_finger;
//             pshape->ds_array.push_back(true);
//             partial_placement.push_back(pshape);
//         }
//         current_node->partial_shapes = partial_placement;
//         // std::cout << "pass" << std::endl;
//         // std::cout << "current_node->tr->name: " << current_node->tr->name << std::endl;
//         // std::cout << current_tr->name << " partial_placement.size(): " << partial_placement.size() << std::endl;
//         while (true) {
//             if (pruned) {
//                 // std::cout << "while loop" << std::endl;
//                 // std::cout << current_node->tr->name << std::endl;
//                 pruned = false;
//                 while (true) {
//                     Node* n = current_node->parent;
//                     tr_left_sum = tr_left_sum + current_tr->num_finger;
//                     // std::cout << "tr_left_sum: " << tr_left_sum << std::endl;
//                     // std::cout << "current_node->fill: " << current_node->fill << " " << current_node->remained_pmos.size() - 1 << std::endl;
//                     for (auto pshape: current_node->partial_shapes) {
//                         delete pshape;
//                     }
//                     delete current_node;
//                     current_node = n;
//                     current_tr = n->tr;
//                     // std::cout << current_node->tr->name << std::endl;
//                     if (((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) == false) {
//                         break;
//                     }
//                 }
//                 // std::cout << "while loop finish" << std::endl;
//                 current_node->fill++;
//                 if (current_node == root) {
//                     break;
//                 }
//             }
//             else if (current_node->remained_pmos.size() == 0) {
//                 if (current_node->partial_shapes.size() != 0) {
//                     if (current_node->partial_shapes[0]->width < min_cell_width) {
//                         min_cell_width = current_node->partial_shapes[0]->width;
//                         placement_cand = current_node->partial_shapes;
//                         std::cout << "min_cell_width: " << min_cell_width << std::endl;
//                         // for (auto item: current_node->partial_shapes) {
//                         //     placement_cand.push_back(item);
//                         // }
//                     }
//                     else if (current_node->partial_shapes[0]->width == min_cell_width) {
//                         // std::cout << "partial_placement.size(): " << partial_placement.size() << std::endl;
//                         // if (placement_cand.size() < 64) {
//                         if (placement_cand.size() < 10000) {
//                             for (auto item: current_node->partial_shapes) {
//                                 placement_cand.push_back(item);
//                             }
//                         }
//                         // std::cout << current_node->tr->name << " min_cell_width: " << min_cell_width << std::endl;
//                     }
//                 }
//                 // std::cout << "leaf" << std::endl;
//                 // std::cout << "while loop" << std::endl;
//                 // std::cout << current_node->tr->name << std::endl;
//                 while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
//                     Node* n = current_node->parent;
//                     tr_left_sum = tr_left_sum + current_tr->num_finger;
//                     delete current_node;
//                     current_node = n;
//                     current_tr = n->tr;
//                     // std::cout << "tr_left_sum: " << tr_left_sum << std::endl;
//                 }
//                 // std::cout << "while loop finish" << std::endl;
//                 current_node->fill++;
//                 if (current_node == root) {
//                     break;
//                 }
//             }
//             else {
//                 if (current_node->partial_shapes.size() == 0) {
//                     // for (auto ptr : current_node->partial_shapes) {
//                     //     delete ptr;  // 釋放每個動態分配的 Node 物件
//                     // }
//                     current_node->partial_shapes.clear();
//                     current_node->partial_shapes.shrink_to_fit();
//                     pruned = true;
//                     // std::cout << "pruned!" << std::endl;
//                     // std::cout << std::endl;
//                     continue;
//                 }
//                 Node* n = new Node();
//                 std::vector<Transistor*> remained_pmos = current_node->remained_pmos;
//                 int config_idx = 0;
//                 Transistor* old_tr = current_tr;
//                 current_tr = remained_pmos[current_node->fill];
//                 // std::cout << old_tr->name << " " << current_tr->name << std::endl;
//                 std::vector<Pshape*> new_partial_placement;
//                 int min_partial_width = current_node->partial_shapes[0]->width + current_tr->num_finger+ 1;
//                 int low_bound;
//                 tr_left_sum = tr_left_sum - current_tr->num_finger;
                
//                 for (auto old_pshape: current_node->partial_shapes) {
//                     // int config_idx = 0;
//                     for (int config_idx = 0; config_idx <= 1; config_idx++) {
//                         Pshape* pshape = new Pshape();
//                         auto new_tr_permutation = old_pshape->tr_permutaton;
//                         auto new_tr_shape_id = old_pshape->tr_shape_id;
//                         auto new_ds_array = old_pshape->ds_array;
//                         // std::cout << old_tr->name << " old_pshape->tr_shape_id.size(): " << old_pshape->tr_shape_id.size() << std::endl;
//                         // bool ds = diffusion_sharing_single_row(
//                         //     single_row_configs[old_tr][old_pshape->tr_shape_id.back()], 
//                         //     single_row_configs[current_tr][config_idx],
//                         //     old_tr,
//                         //     current_tr);
//                         bool ds = (test_get_right_active(old_tr, old_pshape->tr_shape_id.back()) == test_get_left_active(current_tr, config_idx));
//                         // bool ds = true;
//                         // std::cout << old_tr->name << " " << current_tr->name << " " << old_pshape->tr_shape_id.back() << " " << config_idx << " ds: " << ds << std::endl;
//                         if (ds) {
//                             if (old_pshape->width + current_tr->num_finger < min_partial_width) {
//                                 new_partial_placement = std::vector<Pshape*>();
//                                 min_partial_width = old_pshape->width + current_tr->num_finger;
//                             }
//                             new_tr_permutation.push_back(current_tr);
//                             new_tr_shape_id.push_back(config_idx);
//                             new_ds_array.push_back(true);
//                             pshape->tr_permutaton = new_tr_permutation;
//                             pshape->tr_shape_id = new_tr_shape_id;
//                             pshape->width = old_pshape->width + current_tr->num_finger;
//                             pshape->ds_array = new_ds_array;
//                             new_partial_placement.push_back(pshape);
//                         }
//                         else {
//                             low_bound = old_pshape->width + current_tr->num_finger + 1 + tr_left_sum;
//                             // std::cout << "low_bound: " << low_bound << " tr_left_sum: " << tr_left_sum << std::endl;
//                             if (low_bound > min_cell_width) {
//                             // if (low_bound >= std::min(17, min_cell_width)) {
//                                 delete pshape;
//                                 pshape = nullptr;
//                                 continue;
//                             }
//                             if (min_partial_width < old_pshape->width + current_tr->num_finger + 1) {
//                                 delete pshape;
//                                 pshape = nullptr;
//                                 continue;
//                             }
//                             new_tr_permutation.push_back(current_tr);
//                             new_tr_shape_id.push_back(config_idx);
//                             new_ds_array.push_back(false);
//                             pshape->tr_permutaton = new_tr_permutation;
//                             pshape->tr_shape_id = new_tr_shape_id;
//                             pshape->width = old_pshape->width + current_tr->num_finger + 1;
//                             pshape->ds_array = new_ds_array;
//                             new_partial_placement.push_back(pshape);
//                         }
//                         // config_idx++;
//                     }
//                 }
//                 // std::cout << std::endl;
//                 // for (Pshape* pshape : partial_placement) {
//                 //     delete pshape;  
//                 // }
//                 n->partial_shapes = new_partial_placement;
//                 // std::cout << current_tr->name << " partial_placement.size(): " << partial_placement.size() << std::endl;
//                 // for (auto pshape: n->partial_shapes) {
//                 //     for (int j = 0 ; j < pshape->tr_shape_id.size() ; j++) {
//                 //         std::cout << pshape->tr_shape_id[j] << " ";
//                 //     }
//                 //     std::cout << "width: " << pshape->width << std::endl;
//                 // }

//                 remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), current_tr), remained_pmos.end());
//                 n->remained_pmos = remained_pmos;
//                 n->parent = current_node;
//                 n->tr = current_tr;
//                 n->fill = 0;
//                 current_node->children.push_back(n);
//                 current_node = n;
//                 // std::cout << "current_node->tr->name: " << current_node->tr->name << std::endl;
//             }
//         }
//         std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
//     }

//     // print solution
//     std::cout << "min_cell_width: " << min_cell_width << std::endl;
//     std::cout << "number of cell: " << placement_cand.size() << std::endl;
//     for (auto pshape: placement_cand) {
//         for (int i = 0; i < pshape->tr_permutaton.size(); i++) {
//             Transistor* tr_p = pshape->tr_permutaton[i];
//             // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//             for (int k = 0; k < tr_p->num_finger; k++) {
//                 std::cout << tr_p->name << " ";
//             }
//         }
//         std::cout << std::endl;
//         //print shape idx
//         for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
//             Transistor* tr_p = pshape->tr_permutaton[i];
//             // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//                 for (int k = 0; k < tr_p->num_finger; k++) {
//                     std::cout << pshape->tr_shape_id[i] << " ";
//                 }
//         }
//         std::cout << std::endl;
//         //print active
//         for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
//             Transistor* tr_p = pshape->tr_permutaton[i];
//             // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//             if (pshape->ds_array[i] == false) {
//                 std::cout << " dummy_gate ";
//             } 
//             for (int k = 0; k < tr_p->num_finger; k++) {
//                 if (k % 2 == 0) {
//                     if (pshape->tr_shape_id[i] == 0) {
//                         std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
//                     }
//                     else {
//                         std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
//                     }
//                 }
//                 else {
//                     if (pshape->tr_shape_id[i] == 0) {
//                         std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
//                     }
//                     else {
//                         std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
//                     }
//                 }
//             }
//         }
//         std::cout << std::endl;
//     }
// }

// void CFET::test_nmos() {
//     std::unordered_map<int, Transistor*> tr_id;
//     std::vector<int> nums;
//     std::vector<Pshape*> placement_cand;
//     int tr_idx = 0;
//     int tr_size_sum = 0;
//     int min_cell_width = std::numeric_limits<int>::max();

//     for (Transistor* tr: nmos) {
//         tr_size_sum = tr_size_sum + tr->num_finger;
//     }

//     Node* root = new Node();
//     for (int i = 0; i < nmos.size(); i++) {
//         bool pruned = false;
//         std::cout << "i: " << i << std::endl;
//         Node* current_node = new Node();
//         std::vector<Transistor*> remained_pmos(nmos);
//         Transistor* current_tr = nmos[i];
//         std::vector<Pshape* > partial_placement;
//         int tr_left_sum = tr_size_sum;
//         int config_idx = 0;
//         current_node->parent = root;
//         remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), nmos[i]), remained_pmos.end());
//         current_node->remained_pmos = remained_pmos;
//         current_node->tr = current_tr;
//         current_node->fill = 0;
        
//         tr_left_sum = tr_left_sum - current_tr->num_finger;
//         // std::cout << "tr_left_sum: " << tr_left_sum << std::endl;

//         for (int config_idx = 0; config_idx <= 1; config_idx++) {
//             Pshape* pshape = new Pshape();
//             pshape->tr_permutaton.push_back(current_tr);
//             pshape->tr_shape_id.push_back(config_idx);
//             pshape->width = nmos[i]->num_finger;
//             pshape->ds_array.push_back(true);
//             partial_placement.push_back(pshape);
//         }
//         current_node->partial_shapes = partial_placement;
//         // std::cout << "pass" << std::endl;
//         // std::cout << "current_node->tr->name: " << current_node->tr->name << std::endl;
//         // std::cout << current_tr->name << " partial_placement.size(): " << partial_placement.size() << std::endl;
//         while (true) {
//             if (pruned) {
//                 // std::cout << "while loop" << std::endl;
//                 // std::cout << current_node->tr->name << std::endl;
//                 pruned = false;
//                 while (true) {
//                     Node* n = current_node->parent;
//                     tr_left_sum = tr_left_sum + current_tr->num_finger;
//                     // std::cout << "tr_left_sum: " << tr_left_sum << std::endl;
//                     // std::cout << "current_node->fill: " << current_node->fill << " " << current_node->remained_pmos.size() - 1 << std::endl;
//                     for (auto pshape: current_node->partial_shapes) {
//                         delete pshape;
//                     }
//                     delete current_node;
//                     current_node = n;
//                     current_tr = n->tr;
//                     // std::cout << current_node->tr->name << std::endl;
//                     if (((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) == false) {
//                         break;
//                     }
//                 }
//                 // std::cout << "while loop finish" << std::endl;
//                 current_node->fill++;
//                 if (current_node == root) {
//                     break;
//                 }
//             }
//             else if (current_node->remained_pmos.size() == 0) {
//                 if (current_node->partial_shapes.size() != 0) {
//                     if (current_node->partial_shapes[0]->width < min_cell_width) {
//                         min_cell_width = current_node->partial_shapes[0]->width;
//                         placement_cand = current_node->partial_shapes;
//                         std::cout << "min_cell_width: " << min_cell_width << std::endl;
//                         // for (auto item: current_node->partial_shapes) {
//                         //     placement_cand.push_back(item);
//                         // }
//                     }
//                     else if (current_node->partial_shapes[0]->width == min_cell_width) {
//                         // std::cout << "partial_placement.size(): " << partial_placement.size() << std::endl;
//                         // if (placement_cand.size() < 64) {
//                         if (placement_cand.size() < 10000) {
//                             for (auto item: current_node->partial_shapes) {
//                                 placement_cand.push_back(item);
//                             }
//                         }
//                         // std::cout << current_node->tr->name << " min_cell_width: " << min_cell_width << std::endl;
//                     }
//                 }
//                 // std::cout << "leaf" << std::endl;
//                 // std::cout << "while loop" << std::endl;
//                 // std::cout << current_node->tr->name << std::endl;
//                 while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
//                     Node* n = current_node->parent;
//                     tr_left_sum = tr_left_sum + current_tr->num_finger;
//                     delete current_node;
//                     current_node = n;
//                     current_tr = n->tr;
//                     // std::cout << "tr_left_sum: " << tr_left_sum << std::endl;
//                 }
//                 // std::cout << "while loop finish" << std::endl;
//                 current_node->fill++;
//                 if (current_node == root) {
//                     break;
//                 }
//             }
//             else {
//                 if (current_node->partial_shapes.size() == 0) {
//                     // for (auto ptr : current_node->partial_shapes) {
//                     //     delete ptr;  // 釋放每個動態分配的 Node 物件
//                     // }
//                     current_node->partial_shapes.clear();
//                     current_node->partial_shapes.shrink_to_fit();
//                     pruned = true;
//                     // std::cout << "pruned!" << std::endl;
//                     // std::cout << std::endl;
//                     continue;
//                 }
//                 Node* n = new Node();
//                 std::vector<Transistor*> remained_pmos = current_node->remained_pmos;
//                 int config_idx = 0;
//                 Transistor* old_tr = current_tr;
//                 current_tr = remained_pmos[current_node->fill];
//                 // std::cout << old_tr->name << " " << current_tr->name << std::endl;
//                 std::vector<Pshape*> new_partial_placement;
//                 int min_partial_width = current_node->partial_shapes[0]->width + current_tr->num_finger+ 1;
//                 int low_bound;
//                 tr_left_sum = tr_left_sum - current_tr->num_finger;
                
//                 for (auto old_pshape: current_node->partial_shapes) {
//                     // int config_idx = 0;
//                     for (int config_idx = 0; config_idx <= 1; config_idx++) {
//                         Pshape* pshape = new Pshape();
//                         auto new_tr_permutation = old_pshape->tr_permutaton;
//                         auto new_tr_shape_id = old_pshape->tr_shape_id;
//                         auto new_ds_array = old_pshape->ds_array;
//                         // std::cout << old_tr->name << " old_pshape->tr_shape_id.size(): " << old_pshape->tr_shape_id.size() << std::endl;
//                         // bool ds = diffusion_sharing_single_row(
//                         //     single_row_configs[old_tr][old_pshape->tr_shape_id.back()], 
//                         //     single_row_configs[current_tr][config_idx],
//                         //     old_tr,
//                         //     current_tr);
//                         bool ds = (test_get_right_active(old_tr, old_pshape->tr_shape_id.back()) == test_get_left_active(current_tr, config_idx));
//                         // bool ds = true;
//                         // std::cout << old_tr->name << " " << current_tr->name << " " << old_pshape->tr_shape_id.back() << " " << config_idx << " ds: " << ds << std::endl;
//                         if (ds) {
//                             if (old_pshape->width + current_tr->num_finger < min_partial_width) {
//                                 new_partial_placement = std::vector<Pshape*>();
//                                 min_partial_width = old_pshape->width + current_tr->num_finger;
//                             }
//                             new_tr_permutation.push_back(current_tr);
//                             new_tr_shape_id.push_back(config_idx);
//                             new_ds_array.push_back(true);
//                             pshape->tr_permutaton = new_tr_permutation;
//                             pshape->tr_shape_id = new_tr_shape_id;
//                             pshape->width = old_pshape->width + current_tr->num_finger;
//                             pshape->ds_array = new_ds_array;
//                             new_partial_placement.push_back(pshape);
//                         }
//                         else {
//                             low_bound = old_pshape->width + current_tr->num_finger + 1 + tr_left_sum;
//                             // std::cout << "low_bound: " << low_bound << " tr_left_sum: " << tr_left_sum << std::endl;
//                             if (low_bound > min_cell_width) {
//                             // if (low_bound >= std::min(17, min_cell_width)) {
//                                 delete pshape;
//                                 pshape = nullptr;
//                                 continue;
//                             }
//                             if (min_partial_width < old_pshape->width + current_tr->num_finger + 1) {
//                                 delete pshape;
//                                 pshape = nullptr;
//                                 continue;
//                             }
//                             new_tr_permutation.push_back(current_tr);
//                             new_tr_shape_id.push_back(config_idx);
//                             new_ds_array.push_back(false);
//                             pshape->tr_permutaton = new_tr_permutation;
//                             pshape->tr_shape_id = new_tr_shape_id;
//                             pshape->width = old_pshape->width + current_tr->num_finger + 1;
//                             pshape->ds_array = new_ds_array;
//                             new_partial_placement.push_back(pshape);
//                         }
//                         // config_idx++;
//                     }
//                 }
//                 // std::cout << std::endl;
//                 // for (Pshape* pshape : partial_placement) {
//                 //     delete pshape;  
//                 // }
//                 n->partial_shapes = new_partial_placement;
//                 // std::cout << current_tr->name << " partial_placement.size(): " << partial_placement.size() << std::endl;
//                 // for (auto pshape: n->partial_shapes) {
//                 //     for (int j = 0 ; j < pshape->tr_shape_id.size() ; j++) {
//                 //         std::cout << pshape->tr_shape_id[j] << " ";
//                 //     }
//                 //     std::cout << "width: " << pshape->width << std::endl;
//                 // }

//                 remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), current_tr), remained_pmos.end());
//                 n->remained_pmos = remained_pmos;
//                 n->parent = current_node;
//                 n->tr = current_tr;
//                 n->fill = 0;
//                 current_node->children.push_back(n);
//                 current_node = n;
//                 // std::cout << "current_node->tr->name: " << current_node->tr->name << std::endl;
//             }
//         }
//         std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
//     }

//     // print solution
//     std::cout << "min_cell_width: " << min_cell_width << std::endl;
//     std::cout << "number of cell: " << placement_cand.size() << std::endl;
//     for (auto pshape: placement_cand) {
//         for (int i = 0; i < pshape->tr_permutaton.size(); i++) {
//             Transistor* tr_p = pshape->tr_permutaton[i];
//             // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//             for (int k = 0; k < tr_p->num_finger; k++) {
//                 std::cout << tr_p->name << " ";
//             }
//         }
//         std::cout << std::endl;
//         //print shape idx
//         for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
//             Transistor* tr_p = pshape->tr_permutaton[i];
//             // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//                 for (int k = 0; k < tr_p->num_finger; k++) {
//                     std::cout << pshape->tr_shape_id[i] << " ";
//                 }
//         }
//         std::cout << std::endl;
//         //print active
//         for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
//             Transistor* tr_p = pshape->tr_permutaton[i];
//             // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//             if (pshape->ds_array[i] == false) {
//                 std::cout << " dummy_gate ";
//             } 
//             for (int k = 0; k < tr_p->num_finger; k++) {
//                 if (k % 2 == 0) {
//                     if (pshape->tr_shape_id[i] == 0) {
//                         std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
//                     }
//                     else {
//                         std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
//                     }
//                 }
//                 else {
//                     if (pshape->tr_shape_id[i] == 0) {
//                         std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
//                     }
//                     else {
//                         std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
//                     }
//                 }
//             }
//         }
//         std::cout << std::endl;
//     }
// }

// CFET::Signal* CFET::test_get_right_active(Transistor* tr, int shape_idx) {
//     return (tr->num_finger % 2 == 1) ? (shape_idx == 0) ? tr->source : tr->drain : (shape_idx == 0) ? tr->drain : tr->source;
// }

// CFET::Signal* CFET::test_get_left_active(Transistor* tr, int shape_idx) {
//     return (shape_idx == 0) ? tr->drain : tr->source;
// }