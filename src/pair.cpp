#include <chrono>
#include <iomanip>
#include <iostream>
#include <queue>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "cfet.h"

void new_tr_pairing() {
    // find neighbors of transistors
    for (auto tr : trs) {
        for (auto tr2 : trs) {
            if (tr->source == tr2->drain && tr->source->name != "VDD" && tr->source->name != "VSS" ||
                tr->drain == tr2->source && tr2->source->name != "VDD" && tr2->source->name != "VSS") {
                if (tr != tr2) {
                    tr->neighbors.push_back(tr2);
                }
            }
        }
    }
    // std::cout << "tr neighbors: " << std::endl;
    // for (auto tr: trs) {
    //     std::cout << tr->name << std::endl;
    //     for (auto tr2: tr->neighbors) {
    //         std::cout << tr2->name << std::endl;
    //     }
    //     std::cout << std::endl;
    // }
    // identify all transmission gates
    std::vector<bool> pmos_visited;
    std::vector<bool> nmos_visited;
    pmos_visited.assign(pmos.size(), false);
    nmos_visited.assign(nmos.size(), false);
    std::vector<std::vector<Transistor*>> transmission_gates;
    for (int i = 0; i < pmos.size(); i++) {
        Transistor* tr_p = pmos[i];
        for (int j = 0; j < nmos.size(); j++) {
            Transistor* tr_n = nmos[j];
            if (tr_p->drain == tr_n->drain && tr_p->source == tr_n->source) {
                tr_pairs[tr_p] = tr_n;
                pmos_visited[i] = true;
                nmos_visited[j] = true;
                std::vector<Transistor*> group;
                group.push_back(tr_p);
                group.push_back(tr_n);
                transmission_gates.push_back(group);
                break;
            }
        }
    }
    // for (int i = 0; i < pmos.size(); i++) {
    //     std::cout << pmos[i]->name << " " << pmos_visited[i] << std::endl;
    // }
    // for (int i = 0; i < nmos.size(); i++) {
    //     std::cout << nmos[i]->name << " " << nmos_visited[i] << std::endl;
    // }
    // find all primary output node
    // std::cout << "find all primary output node" << std::endl;
    std::vector<Signal*> nets;
    for (auto pair : signals) {
        if (pair.first != "VDD" && pair.first != "VSS") {
            nets.push_back(pair.second);
        }
    }
    std::vector<std::vector<Transistor*>> compound_gates;
    for (auto net : nets) {
        bool pmos_share_net = false;
        bool nmos_share_net = false;
        std::vector<Transistor*> group;
        for (int i = 0; i < pmos.size(); i++) {
            Transistor* tr_p = pmos[i];
            if (tr_p->drain == net || tr_p->source == net) {
                if (pmos_visited[i] == false) {
                    group.push_back(tr_p);
                    pmos_share_net = true;
                }
            }
        }
        for (int i = 0; i < nmos.size(); i++) {
            Transistor* tr_n = nmos[i];
            if (tr_n->drain == net || tr_n->source == net) {
                if (nmos_visited[i] == false) {
                    group.push_back(tr_n);
                    nmos_share_net = true;
                }
            }
        }
        if (pmos_share_net && nmos_share_net) {
            // std::cout << net->name << std::endl;
            compound_gates.push_back(group);
        }
    }
    std::cout << "compound_gates: " << std::endl;
    for (auto group : compound_gates) {
        std::cout << std::endl;
        for (auto tr : group) {
            std::cout << tr->name << std::endl;
        }
    }
    std::cout << std::endl;
    std::cout << "transmission_gates: " << std::endl;
    for (auto group : transmission_gates) {
        std::cout << std::endl;
        for (auto tr : group) {
            std::cout << tr->name << std::endl;
        }
    }
    // pair for each group
    // std::cout << std::endl;
    std::vector<Transistor*> _pmos;
    std::vector<Transistor*> _nmos;
    _pmos = pmos;
    _nmos = nmos;
    for (auto group : compound_gates) {
        // find p-network and n-network
        // std::cout << "find p-network and n-network" << std::endl;
        std::vector<Transistor*> p_network;
        std::vector<Transistor*> n_network;
        std::unordered_map<Transistor*, bool> mos_visited_in_group;
        for (auto tr : trs) {
            mos_visited_in_group.insert(std::make_pair(tr, false));
        }
        for (auto group : transmission_gates) {
            for (auto tr : group) {
                mos_visited_in_group[tr] = true;
            }
        }
        for (auto tr : group) {
            std::queue<Transistor*> q;
            if (tr->type == MosType::PMOS) {
                if (mos_visited_in_group[tr] == false) {
                    // std::cout << tr->name << " pmos" << std::endl;
                    q.push(tr);
                    mos_visited_in_group[tr] = true;
                    p_network.push_back(tr);
                    while (!q.empty()) {
                        Transistor* current_tr = q.front();
                        // std::cout << "current_tr(pmos): " << current_tr->name << std::endl;
                        for (auto neighbor_tr : current_tr->neighbors) {
                            if (mos_visited_in_group[neighbor_tr] == false && neighbor_tr->type == MosType::PMOS) {
                                q.push(neighbor_tr);
                                mos_visited_in_group[neighbor_tr] = true;
                                // std::cout << "push back: " << neighbor_tr->name << std::endl;
                                p_network.push_back(neighbor_tr);
                            }
                        }
                        q.pop();
                    }
                }
            } else {
                if (mos_visited_in_group[tr] == false) {
                    // std::cout << tr->name << " nmos" << std::endl;
                    q.push(tr);
                    mos_visited_in_group[tr] = true;
                    n_network.push_back(tr);
                    while (!q.empty()) {
                        Transistor* current_tr = q.front();
                        // std::cout << "current_tr(nmos): " << current_tr->name << std::endl;
                        for (auto neighbor_tr : current_tr->neighbors) {
                            if (mos_visited_in_group[neighbor_tr] == false && neighbor_tr->type == MosType::NMOS) {
                                q.push(neighbor_tr);
                                mos_visited_in_group[neighbor_tr] = true;
                                // std::cout << "push back: " << neighbor_tr->name << std::endl;
                                n_network.push_back(neighbor_tr);
                            }
                        }
                        q.pop();
                    }
                }
            }
        }
        std::cout << "p-network: " << std::endl;
        for (auto tr_p : p_network) {
            std::cout << tr_p->name << std::endl;
        }
        std::cout << "n-network: " << std::endl;
        for (auto tr_n : n_network) {
            std::cout << tr_n->name << std::endl;
        }
        std::cout << std::endl;
        // do pairing
        for (auto tr_p : p_network) {
            // find same size
            std::vector<Transistor*> same_size_trs;
            for (auto tr_n : n_network) {
                if (tr_n->width == tr_p->width) {
                    // if (tr_n->num_finger == tr_p->num_finger) {
                    same_size_trs.push_back(tr_n);
                }
            }
            if (same_size_trs.size() != 0) {
                // find tr_n with most common signals
                Transistor* tr_paired = same_size_trs[0];
                int max_common_signal = 0;
                for (auto tr_n : same_size_trs) {
                    std::vector<Signal*> tr_n_signals;
                    tr_n_signals.push_back(tr_n->drain);
                    tr_n_signals.push_back(tr_n->gate);
                    tr_n_signals.push_back(tr_n->source);
                    int d = std::count(tr_n_signals.begin(), tr_n_signals.end(), tr_p->drain);
                    int g = std::count(tr_n_signals.begin(), tr_n_signals.end(), tr_p->gate);
                    int s = std::count(tr_n_signals.begin(), tr_n_signals.end(), tr_p->source);
                    int same_size = (tr_n->width == tr_p->width) ? 1 : 0;
                    if (d + g + s > max_common_signal) {
                        max_common_signal = d + g + s;
                        tr_paired = tr_n;
                    }
                    // if (2 * d + 2 * g + 2 * s + same_size > max_common_signal) {
                    //     max_common_signal = 2 * d + 2 * g + 2 * s + same_size;
                    //     tr_paired = tr_n;
                    // }
                }
                tr_pairs[tr_p] = tr_paired;
                n_network.erase(std::remove(n_network.begin(), n_network.end(), tr_paired), n_network.end());

                _pmos.erase(std::remove(_pmos.begin(), _pmos.end(), tr_p), _pmos.end());
                _nmos.erase(std::remove(_nmos.begin(), _nmos.end(), tr_paired), _nmos.end());
            } else {
                // std::cout << "unpaired: " << tr_p->name << std::endl;
                Transistor* tr_paired = n_network[0];
                int max_common_signal = 0;
                for (auto tr_n : n_network) {
                    std::vector<Signal*> tr_n_signals;
                    tr_n_signals.push_back(tr_n->drain);
                    tr_n_signals.push_back(tr_n->gate);
                    tr_n_signals.push_back(tr_n->source);
                    int d = std::count(tr_n_signals.begin(), tr_n_signals.end(), tr_p->drain);
                    int g = std::count(tr_n_signals.begin(), tr_n_signals.end(), tr_p->gate);
                    int s = std::count(tr_n_signals.begin(), tr_n_signals.end(), tr_p->source);
                    int same_size = (tr_n->width == tr_p->width) ? 1 : 0;
                    if (d + g + s > max_common_signal) {
                        max_common_signal = d + g + s;
                        tr_paired = tr_n;
                    }
                    // if (2 * d + 2 * g + 2 * s + same_size > max_common_signal) {
                    //     max_common_signal = 2 * d + 2 * g + 2 * s + same_size;
                    //     tr_paired = tr_n;
                    // }
                }
                tr_pairs[tr_p] = tr_paired;
                n_network.erase(std::remove(n_network.begin(), n_network.end(), tr_paired), n_network.end());

                _pmos.erase(std::remove(_pmos.begin(), _pmos.end(), tr_p), _pmos.end());
                _nmos.erase(std::remove(_nmos.begin(), _nmos.end(), tr_paired), _nmos.end());
            }
        }
    }
    // std::cout << _pmos.size() << " " << _nmos.size() << std::endl;
    // if (_pmos.size() != 0) {
    //     for (auto tr_p: _pmos) {
    //         Transistor* tr_null = new Transistor();
    //         tr_null->name = "NULL";
    //         tr_null->width = 0;
    //         tr_null->drain = nullptr;
    //         tr_null->gate = nullptr;
    //         tr_null->source = nullptr;
    //         tr_null->type = MosType::NMOS;
    //         tr_pairs[tr_p] = tr_null;
    //     }
    // }
    // else if (_nmos.size() != 0) {
    //     for (auto tr_n: _nmos) {
    //         Transistor* tr_null = new Transistor();
    //         tr_null->name = "NULL";
    //         tr_null->width = 0;
    //         tr_null->drain = nullptr;
    //         tr_null->gate = nullptr;
    //         tr_null->source = nullptr;
    //         tr_null->type = MosType::NMOS;
    //         tr_pairs[tr_null] = tr_n;
    //     }
    // }
    // print pairs
    std::cout << "pairs: " << std::endl;
    for (auto pair : tr_pairs) {
        std::cout << pair.first->name << " " << pair.second->name << std::endl;
    }
}

void custom_pairing() {
    std::cout << "custom pairing" << std::endl;
    // tr_pairs[tr_dict["MM22"]] = tr_dict["MM25"];
    // FAx1
    // tr_pairs[tr_dict["MM22"]] = tr_dict["MM25"];
    // tr_pairs[tr_dict["MM21"]] = tr_dict["MM24"];
    // tr_pairs[tr_dict["MM20"]] = tr_dict["MM23"];
    // tr_pairs[tr_dict["MM15"]] = tr_dict["MM16"];
    // tr_pairs[tr_dict["MM14"]] = tr_dict["MM19"];
    // tr_pairs[tr_dict["MM13"]] = tr_dict["MM18"];
    // tr_pairs[tr_dict["MM12"]] = tr_dict["MM17"];
    // tr_pairs[tr_dict["MM5"]] = tr_dict["MM8"];
    // tr_pairs[tr_dict["MM6"]] = tr_dict["MM10"];
    // tr_pairs[tr_dict["MM2"]] = tr_dict["MM11"];
    // tr_pairs[tr_dict["MM1"]] = tr_dict["MM7"];
    // tr_pairs[tr_dict["MM0"]] = tr_dict["MM9"];
    // DFFHQNx1
    // tr_pairs[tr_dict["MM3"]] = tr_dict["MM5"];
    // tr_pairs[tr_dict["MM21"]] = tr_dict["MM20"];
    // tr_pairs[tr_dict["MM22"]] = tr_dict["MM23"];
    // tr_pairs[tr_dict["MM25"]] = tr_dict["MM24"];
    // tr_pairs[tr_dict["MM15"]] = tr_dict["MM14"];
    // tr_pairs[tr_dict["MM7"]] = tr_dict["MM6"];
    // tr_pairs[tr_dict["MM18"]] = tr_dict["MM17"];
    // tr_pairs[tr_dict["MM1"]] = tr_dict["MM4"];
    // tr_pairs[tr_dict["MM11"]] = tr_dict["MM8"];
    // tr_pairs[tr_dict["MM10"]] = tr_dict["MM9"];
    // tr_pairs[tr_dict["MM13"]] = tr_dict["MM12"];
    // tr_pairs[tr_dict["MM19"]] = tr_dict["MM16"];
}

// std::vector<CFET::Pshape*> CFET::pmos_placement() {
//     std::unordered_map<int, Transistor*> tr_id;
//     std::vector<int> nums;
//     std::vector<Pshape*> placement_cand;
//     int tr_idx = 0;
//     int tr_size_sum = 0;
//     int min_cell_width = std::numeric_limits<int>::max();

//     for (Transistor* tr : pmos) {
//         tr_size_sum = tr_size_sum + tr->num_finger;
//     }

//     Node* root = new Node();
//     for (int i = 0; i < pmos.size(); i++) {
//         bool pruned = false;
//         // std::cout << "i: " << i << std::endl;
//         Node* current_node = new Node();
//         std::vector<Transistor*> remained_pmos(pmos);
//         Transistor* current_tr = pmos[i];
//         std::vector<Pshape*> partial_placement;
//         int tr_left_sum = tr_size_sum;
//         int config_idx = 0;
//         current_node->parent = root;
//         remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), pmos[i]), remained_pmos.end());
//         current_node->remained_pmos = remained_pmos;
//         current_node->tr = current_tr;
//         current_node->fill = 0;

//         tr_left_sum = tr_left_sum - current_tr->num_finger;

//         for (int config_idx = 0; config_idx <= 1; config_idx++) {
//             Pshape* pshape = new Pshape();
//             pshape->tr_permutaton.push_back(current_tr);
//             pshape->tr_shape_id.push_back(config_idx);
//             pshape->width = pmos[i]->num_finger;
//             pshape->ds_array.push_back(true);
//             partial_placement.push_back(pshape);
//         }
//         current_node->partial_shapes = partial_placement;
//         while (true) {
//             if (pruned) {
//                 pruned = false;
//                 while (true) {
//                     Node* n = current_node->parent;
//                     tr_left_sum = tr_left_sum + current_tr->num_finger;
//                     for (auto pshape : current_node->partial_shapes) {
//                         delete pshape;
//                     }
//                     delete current_node;
//                     current_node = n;
//                     current_tr = n->tr;
//                     if (((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root))
//                     ==
//                         false) {
//                         break;
//                     }
//                 }
//                 current_node->fill++;
//                 if (current_node == root) {
//                     break;
//                 }
//             } else if (current_node->remained_pmos.size() == 0) {
//                 if (current_node->partial_shapes.size() != 0) {
//                     if (current_node->partial_shapes[0]->width < min_cell_width) {
//                         min_cell_width = current_node->partial_shapes[0]->width;
//                         for (auto item : placement_cand) {
//                             delete item;
//                             item = nullptr;
//                         }
//                         placement_cand.clear();
//                         placement_cand = current_node->partial_shapes;
//                         // std::cout << "min_cell_width: " << min_cell_width << std::endl;
//                     } else if (current_node->partial_shapes[0]->width == min_cell_width) {
//                         if (placement_cand.size() < max_placement_size) {
//                             for (auto item : current_node->partial_shapes) {
//                                 placement_cand.push_back(item);
//                             }
//                         } else {
//                             for (auto item : current_node->partial_shapes) {
//                                 delete item;
//                             }
//                         }
//                         delete current_node;
//                     }
//                 }
//                 while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
//                     Node* n = current_node->parent;
//                     tr_left_sum = tr_left_sum + current_tr->num_finger;
//                     current_node = n;
//                     current_tr = n->tr;
//                 }
//                 current_node->fill++;
//                 if (current_node == root) {
//                     break;
//                 }
//             } else {
//                 if (current_node->partial_shapes.size() == 0) {
//                     current_node->partial_shapes.clear();
//                     current_node->partial_shapes.shrink_to_fit();
//                     pruned = true;
//                     continue;
//                 }
//                 Node* n = new Node();
//                 std::vector<Transistor*> remained_pmos = current_node->remained_pmos;
//                 int config_idx = 0;
//                 Transistor* old_tr = current_tr;
//                 current_tr = remained_pmos[current_node->fill];
//                 std::vector<Pshape*> new_partial_placement;
//                 int min_partial_width = current_node->partial_shapes[0]->width + current_tr->num_finger + 1;
//                 int low_bound;
//                 tr_left_sum = tr_left_sum - current_tr->num_finger;

//                 for (auto old_pshape : current_node->partial_shapes) {
//                     for (int config_idx = 0; config_idx <= 1; config_idx++) {
//                         Pshape* pshape = new Pshape();
//                         auto new_tr_permutation = old_pshape->tr_permutaton;
//                         auto new_tr_shape_id = old_pshape->tr_shape_id;
//                         auto new_ds_array = old_pshape->ds_array;
//                         bool ds = (simple_get_right_active(old_tr, old_pshape->tr_shape_id.back()) == simple_get_left_active(current_tr, config_idx));
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
//                         } else {
//                             low_bound = old_pshape->width + current_tr->num_finger + 1 + tr_left_sum;
//                             if (low_bound > min_cell_width) {
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
//                     }
//                 }
//                 n->partial_shapes = new_partial_placement;

//                 remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), current_tr), remained_pmos.end());
//                 n->remained_pmos = remained_pmos;
//                 n->parent = current_node;
//                 n->tr = current_tr;
//                 n->fill = 0;
//                 current_node->children.push_back(n);
//                 current_node = n;
//             }
//         }
//         // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
//     }

//     // print solution
//     // std::cout << "min_cell_width: " << min_cell_width << std::endl;
//     // std::cout << "number of cell: " << placement_cand.size() << std::endl;
//     for (auto pshape : placement_cand) {
//         // for (int i = 0; i < pshape->tr_permutaton.size(); i++) {
//         //     Transistor* tr_p = pshape->tr_permutaton[i];
//         //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//         //     for (int k = 0; k < tr_p->num_finger; k++) {
//         //         std::cout << tr_p->name << " ";
//         //     }
//         // }
//         // std::cout << std::endl;
//         // print shape idx
//         // for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
//         //     Transistor* tr_p = pshape->tr_permutaton[i];
//         //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//         //         for (int k = 0; k < tr_p->num_finger; k++) {
//         //             std::cout << pshape->tr_shape_id[i] << " ";
//         //         }
//         // }
//         // std::cout << std::endl;
//         // print active
//         // for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
//         //     Transistor* tr_p = pshape->tr_permutaton[i];
//         //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//         //     if (pshape->ds_array[i] == false) {
//         //         std::cout << " dummy_gate ";
//         //     }
//         //     for (int k = 0; k < tr_p->num_finger; k++) {
//         //         if (k % 2 == 0) {
//         //             if (pshape->tr_shape_id[i] == 0) {
//         //                 std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " <<
//         std::left << std::setw(7) << tr_p->source->name << " ";
//         //             }
//         //             else {
//         //                 std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " <<
//         std::left << std::setw(7) << tr_p->drain->name << " ";
//         //             }
//         //         }
//         //         else {
//         //             if (pshape->tr_shape_id[i] == 0) {
//         //                 std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " <<
//         std::left << std::setw(7) << tr_p->drain->name << " ";
//         //             }
//         //             else {
//         //                 std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " <<
//         std::left << std::setw(7) << tr_p->source->name << " ";
//         //             }
//         //         }
//         //     }
//         // }
//         // std::cout << std::endl;
//     }
//     return placement_cand;
// }

// std::vector<CFET::Pshape*> CFET::nmos_placement() {
//     std::unordered_map<int, Transistor*> tr_id;
//     std::vector<int> nums;
//     std::vector<Pshape*> placement_cand;
//     int tr_idx = 0;
//     int tr_size_sum = 0;
//     int min_cell_width = std::numeric_limits<int>::max();

//     for (Transistor* tr : nmos) {
//         tr_size_sum = tr_size_sum + tr->num_finger;
//     }

//     Node* root = new Node();
//     for (int i = 0; i < nmos.size(); i++) {
//         bool pruned = false;
//         // std::cout << "i: " << i << std::endl;
//         Node* current_node = new Node();
//         std::vector<Transistor*> remained_pmos(nmos);
//         Transistor* current_tr = nmos[i];
//         std::vector<Pshape*> partial_placement;
//         int tr_left_sum = tr_size_sum;
//         int config_idx = 0;
//         current_node->parent = root;
//         remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), nmos[i]), remained_pmos.end());
//         current_node->remained_pmos = remained_pmos;
//         current_node->tr = current_tr;
//         current_node->fill = 0;

//         tr_left_sum = tr_left_sum - current_tr->num_finger;

//         for (int config_idx = 0; config_idx <= 1; config_idx++) {
//             Pshape* pshape = new Pshape();
//             pshape->tr_permutaton.push_back(current_tr);
//             pshape->tr_shape_id.push_back(config_idx);
//             pshape->width = nmos[i]->num_finger;
//             pshape->ds_array.push_back(true);
//             partial_placement.push_back(pshape);
//         }
//         current_node->partial_shapes = partial_placement;
//         while (true) {
//             if (pruned) {
//                 pruned = false;
//                 while (true) {
//                     Node* n = current_node->parent;
//                     tr_left_sum = tr_left_sum + current_tr->num_finger;
//                     for (auto pshape : current_node->partial_shapes) {
//                         delete pshape;
//                     }
//                     delete current_node;
//                     current_node = n;
//                     current_tr = n->tr;
//                     if (((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root))
//                     ==
//                         false) {
//                         break;
//                     }
//                 }
//                 current_node->fill++;
//                 if (current_node == root) {
//                     break;
//                 }
//             } else if (current_node->remained_pmos.size() == 0) {
//                 if (current_node->partial_shapes.size() != 0) {
//                     if (current_node->partial_shapes[0]->width < min_cell_width) {
//                         min_cell_width = current_node->partial_shapes[0]->width;
//                         for (auto item : placement_cand) {
//                             delete item;
//                             item = nullptr;
//                         }
//                         placement_cand.clear();
//                         placement_cand = current_node->partial_shapes;
//                         // std::cout << "min_cell_width: " << min_cell_width << std::endl;
//                     } else if (current_node->partial_shapes[0]->width == min_cell_width) {
//                         if (placement_cand.size() < max_placement_size) {
//                             for (auto item : current_node->partial_shapes) {
//                                 placement_cand.push_back(item);
//                             }
//                         } else {
//                             for (auto item : current_node->partial_shapes) {
//                                 delete item;
//                             }
//                         }
//                         delete current_node;
//                     }
//                 }
//                 while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
//                     Node* n = current_node->parent;
//                     tr_left_sum = tr_left_sum + current_tr->num_finger;
//                     current_node = n;
//                     current_tr = n->tr;
//                 }
//                 current_node->fill++;
//                 if (current_node == root) {
//                     break;
//                 }
//             } else {
//                 if (current_node->partial_shapes.size() == 0) {
//                     current_node->partial_shapes.clear();
//                     current_node->partial_shapes.shrink_to_fit();
//                     pruned = true;
//                     continue;
//                 }
//                 Node* n = new Node();
//                 std::vector<Transistor*> remained_pmos = current_node->remained_pmos;
//                 int config_idx = 0;
//                 Transistor* old_tr = current_tr;
//                 current_tr = remained_pmos[current_node->fill];
//                 std::vector<Pshape*> new_partial_placement;
//                 int min_partial_width = current_node->partial_shapes[0]->width + current_tr->num_finger + 1;
//                 int low_bound;
//                 tr_left_sum = tr_left_sum - current_tr->num_finger;

//                 for (auto old_pshape : current_node->partial_shapes) {
//                     for (int config_idx = 0; config_idx <= 1; config_idx++) {
//                         Pshape* pshape = new Pshape();
//                         auto new_tr_permutation = old_pshape->tr_permutaton;
//                         auto new_tr_shape_id = old_pshape->tr_shape_id;
//                         auto new_ds_array = old_pshape->ds_array;
//                         bool ds = (simple_get_right_active(old_tr, old_pshape->tr_shape_id.back()) == simple_get_left_active(current_tr, config_idx));
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
//                         } else {
//                             low_bound = old_pshape->width + current_tr->num_finger + 1 + tr_left_sum;
//                             if (low_bound > min_cell_width) {
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
//                     }
//                 }
//                 n->partial_shapes = new_partial_placement;

//                 remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), current_tr), remained_pmos.end());
//                 n->remained_pmos = remained_pmos;
//                 n->parent = current_node;
//                 n->tr = current_tr;
//                 n->fill = 0;
//                 current_node->children.push_back(n);
//                 current_node = n;
//             }
//         }
//         // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
//     }

//     // print solution
//     // std::cout << "min_cell_width: " << min_cell_width << std::endl;
//     // std::cout << "number of cell: " << placement_cand.size() << std::endl;
//     for (auto pshape : placement_cand) {
//         // for (int i = 0; i < pshape->tr_permutaton.size(); i++) {
//         //     Transistor* tr_p = pshape->tr_permutaton[i];
//         //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//         //     for (int k = 0; k < tr_p->num_finger; k++) {
//         //         std::cout << tr_p->name << " ";
//         //     }
//         // }
//         // std::cout << std::endl;
//         // print shape idx
//         // for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
//         //     Transistor* tr_p = pshape->tr_permutaton[i];
//         //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//         //         for (int k = 0; k < tr_p->num_finger; k++) {
//         //             std::cout << pshape->tr_shape_id[i] << " ";
//         //         }
//         // }
//         // std::cout << std::endl;
//         // print active
//         // for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
//         //     Transistor* tr_p = pshape->tr_permutaton[i];
//         //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
//         //     if (pshape->ds_array[i] == false) {
//         //         std::cout << " dummy_gate ";
//         //     }
//         //     for (int k = 0; k < tr_p->num_finger; k++) {
//         //         if (k % 2 == 0) {
//         //             if (pshape->tr_shape_id[i] == 0) {
//         //                 std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " <<
//         std::left << std::setw(7) << tr_p->source->name << " ";
//         //             }
//         //             else {
//         //                 std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " <<
//         std::left << std::setw(7) << tr_p->drain->name << " ";
//         //             }
//         //         }
//         //         else {
//         //             if (pshape->tr_shape_id[i] == 0) {
//         //                 std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " <<
//         std::left << std::setw(7) << tr_p->drain->name << " ";
//         //             }
//         //             else {
//         //                 std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " <<
//         std::left << std::setw(7) << tr_p->source->name << " ";
//         //             }
//         //         }
//         //     }
//         // }
//         // std::cout << std::endl;
//     }
//     return placement_cand;
// }

// CFET::Signal* CFET::simple_get_right_active(Transistor* tr, int shape_idx) {
//     return (tr->num_finger % 2 == 1) ? (shape_idx == 0) ? tr->source : tr->drain : (shape_idx == 0) ? tr->drain : tr->source;
// }

// CFET::Signal* CFET::simple_get_left_active(Transistor* tr, int shape_idx) { return (shape_idx == 0) ? tr->drain : tr->source; }