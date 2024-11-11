#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include <regex>
#include <iomanip>

#include "cfet.h"

void CFET::tr_pairing() {
    std::vector<Transistor*> _pmos;
    std::vector<Transistor*> _nmos;

    for (Transistor* tr: trs) {
        if (tr->type == MosType::PMOS) {
            _pmos.push_back(tr);
        }
        else {
            _nmos.push_back(tr);
        }
    }

    while (_pmos.size() != 0) {
        std::vector<Transistor*> nmos_cand;
        Transistor* p = _pmos[0];
        Transistor* n_paired;
        for (Transistor* n: _nmos) {
            if (n->gate == p->gate) {
                nmos_cand.push_back(n);
            }
        }
        n_paired = nmos_cand[0];
        int max_same_sig_num = 0;
        for (Transistor* n: nmos_cand) {
            int same_sig_num = 0;
            if (n->drain == p->drain || n->drain == p->source) {
                same_sig_num = same_sig_num + 2;
            }
            if (n->source == p->drain || n->source == p->source) {
                same_sig_num = same_sig_num + 2;
            }
            // if (n->drain->name == "VSS" && p->drain->name == "VSS" || n->drain->name == "VSS" && p->source->name == "VSS") {
            //     same_sig_num++;
            // }
            // if (n->source->name == "VSS" && p->drain->name == "VSS" || n->source->name == "VSS" && p->source->name == "VSS") {
            //     same_sig_num++;
            // }
            if ((n->source->name == "VDD" || n->source->name == "VSS" || n->drain->name == "VDD" || n->drain->name == "VSS") 
            && (p->source->name == "VDD" || p->source->name == "VSS" || p->drain->name == "VDD" || p->drain->name == "VSS")) {
                same_sig_num++;
            }
            if (same_sig_num > max_same_sig_num) {
                n_paired = n;
            }
        }
        tr_pairs.insert(std::make_pair(p, n_paired));
        _pmos.erase(std::remove(_pmos.begin(), _pmos.end(), p), _pmos.end());
        _nmos.erase(std::remove(_nmos.begin(), _nmos.end(), n_paired), _nmos.end());
    }
    std::vector<Pshape*> _pmos_placement = pmos_placement();
    std::vector<Pshape*> _nmos_placement = nmos_placement();
    std::vector<std::vector<Signal*>> _pmos_signals;
    std::vector<std::vector<Signal*>> _nmos_signals;
    _pmos_signals.assign(_pmos_placement.size(), std::vector<Signal*>());
    _nmos_signals.assign(_nmos_placement.size(), std::vector<Signal*>());
    for (int i = 0; i < _pmos_placement.size(); i++) {
        auto pshape = _pmos_placement[i];
        for (int j = 0; j < pshape->tr_shape_id.size(); j++) {
            Transistor* tr_p = pshape->tr_permutaton[j];
            // for (int k = 0; k < tr_p->num_finger; k++) {
            for (int k = 0; k < 1; k++) {
                if (k % 2 == 0) {
                    if (pshape->tr_shape_id[j] == 0) {
                        _pmos_signals[i].push_back(tr_p->drain);
                        _pmos_signals[i].push_back(tr_p->gate);
                        _pmos_signals[i].push_back(tr_p->source);
                        // std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
                    }
                    else {
                        _pmos_signals[i].push_back(tr_p->source);
                        _pmos_signals[i].push_back(tr_p->gate);
                        _pmos_signals[i].push_back(tr_p->drain);
                        // std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
                    }
                }
                else {
                    if (pshape->tr_shape_id[j] == 0) {
                        _pmos_signals[i].push_back(tr_p->source);
                        _pmos_signals[i].push_back(tr_p->gate);
                        _pmos_signals[i].push_back(tr_p->drain);
                        // std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
                    }
                    else {
                        _pmos_signals[i].push_back(tr_p->drain);
                        _pmos_signals[i].push_back(tr_p->gate);
                        _pmos_signals[i].push_back(tr_p->source);
                        // std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
                    }
                }
            }
        }
        // std::cout << std::endl;
    }
    for (int i = 0; i < _nmos_placement.size(); i++) {
        auto pshape = _nmos_placement[i];
        for (int j = 0; j < pshape->tr_shape_id.size(); j++) {
            Transistor* tr_p = pshape->tr_permutaton[j];
            // for (int k = 0; k < tr_p->num_finger; k++) {
            for (int k = 0; k < 1; k++) {
                if (k % 2 == 0) {
                    if (pshape->tr_shape_id[j] == 0) {
                        _nmos_signals[i].push_back(tr_p->drain);
                        _nmos_signals[i].push_back(tr_p->gate);
                        _nmos_signals[i].push_back(tr_p->source);
                        // std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
                    }
                    else {
                        _nmos_signals[i].push_back(tr_p->source);
                        _nmos_signals[i].push_back(tr_p->gate);
                        _nmos_signals[i].push_back(tr_p->drain);
                        // std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
                    }
                }
                else {
                    if (pshape->tr_shape_id[j] == 0) {
                        _nmos_signals[i].push_back(tr_p->source);
                        _nmos_signals[i].push_back(tr_p->gate);
                        _nmos_signals[i].push_back(tr_p->drain);
                        // std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
                    }
                    else {
                        _nmos_signals[i].push_back(tr_p->drain);
                        _nmos_signals[i].push_back(tr_p->gate);
                        _nmos_signals[i].push_back(tr_p->source);
                        // std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
                    }
                }
            }
        }
        // std::cout << std::endl;
    }
    // calculate most common permutation
    int max_common_signal_num = 0;
    Pshape* most_common_pmos;
    Pshape* most_common_nmos;
    for (int p_idx = 0; p_idx < _pmos_signals.size(); p_idx++) {
        auto pmos_sig = _pmos_signals[p_idx];
        for (int n_idx = 0; n_idx < _nmos_signals.size(); n_idx++) {
            auto nmos_sig = _nmos_signals[n_idx];
            // std::cout << "nmos_sig.size(): " << nmos_sig.size() << std::endl;
            // std::cout << "pmos_sig.size(): " << pmos_sig.size() << std::endl;
            int common_signal_num = 0;
            for (int i = 0; i < pmos_sig.size(); i++) {
                if (pmos_sig[i] == nmos_sig[i]) {
                    common_signal_num++;
                }
            }
            if (common_signal_num > max_common_signal_num) {
                max_common_signal_num = common_signal_num;
                most_common_pmos = _pmos_placement[p_idx];
                most_common_nmos = _nmos_placement[n_idx];
            }
        }
    }
    // generate pairs
    for (int i = 0; i < most_common_nmos->tr_permutaton.size(); i++) {
        Transistor* tr_p = most_common_pmos->tr_permutaton[i];
        Transistor* tr_n = most_common_nmos->tr_permutaton[i];
        tr_pairs[tr_p] = tr_n;
        std::cout << tr_p->name << " " << tr_n->name << std::endl;
    }
    std::cout << "max_common_signal_num: " << max_common_signal_num << std::endl;
    for (auto item: _pmos_placement) {
        delete item;
    }
    for (auto item: _nmos_placement) {
        delete item;
    }
}

void CFET::custom_pairing() {
    std::cout << "custom pairing" << std::endl;
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

std::vector<CFET::Pshape*> CFET::pmos_placement() {
    std::unordered_map<int, Transistor*> tr_id;
    std::vector<int> nums;
    std::vector<Pshape*> placement_cand;
    int tr_idx = 0;
    int tr_size_sum = 0;
    int min_cell_width = std::numeric_limits<int>::max();

    for (Transistor* tr: pmos) {
        tr_size_sum = tr_size_sum + tr->num_finger;
    }

    Node* root = new Node();
    for (int i = 0; i < pmos.size(); i++) {
        bool pruned = false;
        // std::cout << "i: " << i << std::endl;
        Node* current_node = new Node();
        std::vector<Transistor*> remained_pmos(pmos);
        Transistor* current_tr = pmos[i];
        std::vector<Pshape* > partial_placement;
        int tr_left_sum = tr_size_sum;
        int config_idx = 0;
        current_node->parent = root;
        remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), pmos[i]), remained_pmos.end());
        current_node->remained_pmos = remained_pmos;
        current_node->tr = current_tr;
        current_node->fill = 0;
        
        tr_left_sum = tr_left_sum - current_tr->num_finger;

        for (int config_idx = 0; config_idx <= 1; config_idx++) {
            Pshape* pshape = new Pshape();
            pshape->tr_permutaton.push_back(current_tr);
            pshape->tr_shape_id.push_back(config_idx);
            pshape->width = pmos[i]->num_finger;
            pshape->ds_array.push_back(true);
            partial_placement.push_back(pshape);
        }
        current_node->partial_shapes = partial_placement;
        while (true) {
            if (pruned) {
                pruned = false;
                while (true) {
                    Node* n = current_node->parent;
                    tr_left_sum = tr_left_sum + current_tr->num_finger;
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
                if (current_node->partial_shapes.size() != 0) {
                    if (current_node->partial_shapes[0]->width < min_cell_width) {
                        min_cell_width = current_node->partial_shapes[0]->width;
                        for (auto item: placement_cand) {
                            delete item;
                            item = nullptr;
                        }
                        placement_cand.clear();
                        placement_cand = current_node->partial_shapes;
                        // std::cout << "min_cell_width: " << min_cell_width << std::endl;
                    }
                    else if (current_node->partial_shapes[0]->width == min_cell_width) {
                        if (placement_cand.size() < max_placement_size) {
                            for (auto item: current_node->partial_shapes) {
                                placement_cand.push_back(item);
                            }
                        }
                        else {
                            for (auto item: current_node->partial_shapes) {
                                delete item;
                            }
                        }
                        delete current_node;
                    }
                }
                while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
                    Node* n = current_node->parent;
                    tr_left_sum = tr_left_sum + current_tr->num_finger;
                    current_node = n;
                    current_tr = n->tr;
                }
                current_node->fill++;
                if (current_node == root) {
                    break;
                }
            }
            else {
                if (current_node->partial_shapes.size() == 0) {
                    current_node->partial_shapes.clear();
                    current_node->partial_shapes.shrink_to_fit();
                    pruned = true;
                    continue;
                }
                Node* n = new Node();
                std::vector<Transistor*> remained_pmos = current_node->remained_pmos;
                int config_idx = 0;
                Transistor* old_tr = current_tr;
                current_tr = remained_pmos[current_node->fill];
                std::vector<Pshape*> new_partial_placement;
                int min_partial_width = current_node->partial_shapes[0]->width + current_tr->num_finger+ 1;
                int low_bound;
                tr_left_sum = tr_left_sum - current_tr->num_finger;
                
                for (auto old_pshape: current_node->partial_shapes) {
                    for (int config_idx = 0; config_idx <= 1; config_idx++) {
                        Pshape* pshape = new Pshape();
                        auto new_tr_permutation = old_pshape->tr_permutaton;
                        auto new_tr_shape_id = old_pshape->tr_shape_id;
                        auto new_ds_array = old_pshape->ds_array;
                        bool ds = (simple_get_right_active(old_tr, old_pshape->tr_shape_id.back()) == simple_get_left_active(current_tr, config_idx));
                        if (ds) {
                            if (old_pshape->width + current_tr->num_finger < min_partial_width) {
                                new_partial_placement = std::vector<Pshape*>();
                                min_partial_width = old_pshape->width + current_tr->num_finger;
                            }
                            new_tr_permutation.push_back(current_tr);
                            new_tr_shape_id.push_back(config_idx);
                            new_ds_array.push_back(true);
                            pshape->tr_permutaton = new_tr_permutation;
                            pshape->tr_shape_id = new_tr_shape_id;
                            pshape->width = old_pshape->width + current_tr->num_finger;
                            pshape->ds_array = new_ds_array;
                            new_partial_placement.push_back(pshape);
                        }
                        else {
                            low_bound = old_pshape->width + current_tr->num_finger + 1 + tr_left_sum;
                            if (low_bound > min_cell_width) {
                                delete pshape;
                                pshape = nullptr;
                                continue;
                            }
                            if (min_partial_width < old_pshape->width + current_tr->num_finger + 1) {
                                delete pshape;
                                pshape = nullptr;
                                continue;
                            }
                            new_tr_permutation.push_back(current_tr);
                            new_tr_shape_id.push_back(config_idx);
                            new_ds_array.push_back(false);
                            pshape->tr_permutaton = new_tr_permutation;
                            pshape->tr_shape_id = new_tr_shape_id;
                            pshape->width = old_pshape->width + current_tr->num_finger + 1;
                            pshape->ds_array = new_ds_array;
                            new_partial_placement.push_back(pshape);
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
        // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
    }

    // print solution
    // std::cout << "min_cell_width: " << min_cell_width << std::endl;
    // std::cout << "number of cell: " << placement_cand.size() << std::endl;
    for (auto pshape: placement_cand) {
        // for (int i = 0; i < pshape->tr_permutaton.size(); i++) {
        //     Transistor* tr_p = pshape->tr_permutaton[i];
        //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
        //     for (int k = 0; k < tr_p->num_finger; k++) {
        //         std::cout << tr_p->name << " ";
        //     }
        // }
        // std::cout << std::endl;
        //print shape idx
        // for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
        //     Transistor* tr_p = pshape->tr_permutaton[i];
        //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
        //         for (int k = 0; k < tr_p->num_finger; k++) {
        //             std::cout << pshape->tr_shape_id[i] << " ";
        //         }
        // }
        // std::cout << std::endl;
        //print active
        // for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
        //     Transistor* tr_p = pshape->tr_permutaton[i];
        //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
        //     if (pshape->ds_array[i] == false) {
        //         std::cout << " dummy_gate ";
        //     } 
        //     for (int k = 0; k < tr_p->num_finger; k++) {
        //         if (k % 2 == 0) {
        //             if (pshape->tr_shape_id[i] == 0) {
        //                 std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
        //             }
        //             else {
        //                 std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
        //             }
        //         }
        //         else {
        //             if (pshape->tr_shape_id[i] == 0) {
        //                 std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
        //             }
        //             else {
        //                 std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
        //             }
        //         }
        //     }
        // }
        // std::cout << std::endl;
    }
    return placement_cand;
}

std::vector<CFET::Pshape*> CFET::nmos_placement() {
    std::unordered_map<int, Transistor*> tr_id;
    std::vector<int> nums;
    std::vector<Pshape*> placement_cand;
    int tr_idx = 0;
    int tr_size_sum = 0;
    int min_cell_width = std::numeric_limits<int>::max();

    for (Transistor* tr: nmos) {
        tr_size_sum = tr_size_sum + tr->num_finger;
    }

    Node* root = new Node();
    for (int i = 0; i < nmos.size(); i++) {
        bool pruned = false;
        // std::cout << "i: " << i << std::endl;
        Node* current_node = new Node();
        std::vector<Transistor*> remained_pmos(nmos);
        Transistor* current_tr = nmos[i];
        std::vector<Pshape* > partial_placement;
        int tr_left_sum = tr_size_sum;
        int config_idx = 0;
        current_node->parent = root;
        remained_pmos.erase(std::remove(remained_pmos.begin(), remained_pmos.end(), nmos[i]), remained_pmos.end());
        current_node->remained_pmos = remained_pmos;
        current_node->tr = current_tr;
        current_node->fill = 0;
        
        tr_left_sum = tr_left_sum - current_tr->num_finger;

        for (int config_idx = 0; config_idx <= 1; config_idx++) {
            Pshape* pshape = new Pshape();
            pshape->tr_permutaton.push_back(current_tr);
            pshape->tr_shape_id.push_back(config_idx);
            pshape->width = nmos[i]->num_finger;
            pshape->ds_array.push_back(true);
            partial_placement.push_back(pshape);
        }
        current_node->partial_shapes = partial_placement;
        while (true) {
            if (pruned) {
                pruned = false;
                while (true) {
                    Node* n = current_node->parent;
                    tr_left_sum = tr_left_sum + current_tr->num_finger;
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
                if (current_node->partial_shapes.size() != 0) {
                    if (current_node->partial_shapes[0]->width < min_cell_width) {
                        min_cell_width = current_node->partial_shapes[0]->width;
                        for (auto item: placement_cand) {
                            delete item;
                            item = nullptr;
                        }
                        placement_cand.clear();
                        placement_cand = current_node->partial_shapes;
                        // std::cout << "min_cell_width: " << min_cell_width << std::endl;
                    }
                    else if (current_node->partial_shapes[0]->width == min_cell_width) {
                        if (placement_cand.size() < max_placement_size) {
                            for (auto item: current_node->partial_shapes) {
                                placement_cand.push_back(item);
                            }
                        }
                        else {
                            for (auto item: current_node->partial_shapes) {
                                delete item;
                            }
                        }
                        delete current_node;
                    }
                }
                while ((current_node->remained_pmos.size() == 0 || current_node->fill == current_node->remained_pmos.size() - 1) && (current_node != root)) {
                    Node* n = current_node->parent;
                    tr_left_sum = tr_left_sum + current_tr->num_finger;
                    current_node = n;
                    current_tr = n->tr;
                }
                current_node->fill++;
                if (current_node == root) {
                    break;
                }
            }
            else {
                if (current_node->partial_shapes.size() == 0) {
                    current_node->partial_shapes.clear();
                    current_node->partial_shapes.shrink_to_fit();
                    pruned = true;
                    continue;
                }
                Node* n = new Node();
                std::vector<Transistor*> remained_pmos = current_node->remained_pmos;
                int config_idx = 0;
                Transistor* old_tr = current_tr;
                current_tr = remained_pmos[current_node->fill];
                std::vector<Pshape*> new_partial_placement;
                int min_partial_width = current_node->partial_shapes[0]->width + current_tr->num_finger+ 1;
                int low_bound;
                tr_left_sum = tr_left_sum - current_tr->num_finger;
                
                for (auto old_pshape: current_node->partial_shapes) {
                    for (int config_idx = 0; config_idx <= 1; config_idx++) {
                        Pshape* pshape = new Pshape();
                        auto new_tr_permutation = old_pshape->tr_permutaton;
                        auto new_tr_shape_id = old_pshape->tr_shape_id;
                        auto new_ds_array = old_pshape->ds_array;
                        bool ds = (simple_get_right_active(old_tr, old_pshape->tr_shape_id.back()) == simple_get_left_active(current_tr, config_idx));
                        if (ds) {
                            if (old_pshape->width + current_tr->num_finger < min_partial_width) {
                                new_partial_placement = std::vector<Pshape*>();
                                min_partial_width = old_pshape->width + current_tr->num_finger;
                            }
                            new_tr_permutation.push_back(current_tr);
                            new_tr_shape_id.push_back(config_idx);
                            new_ds_array.push_back(true);
                            pshape->tr_permutaton = new_tr_permutation;
                            pshape->tr_shape_id = new_tr_shape_id;
                            pshape->width = old_pshape->width + current_tr->num_finger;
                            pshape->ds_array = new_ds_array;
                            new_partial_placement.push_back(pshape);
                        }
                        else {
                            low_bound = old_pshape->width + current_tr->num_finger + 1 + tr_left_sum;
                            if (low_bound > min_cell_width) {
                                delete pshape;
                                pshape = nullptr;
                                continue;
                            }
                            if (min_partial_width < old_pshape->width + current_tr->num_finger + 1) {
                                delete pshape;
                                pshape = nullptr;
                                continue;
                            }
                            new_tr_permutation.push_back(current_tr);
                            new_tr_shape_id.push_back(config_idx);
                            new_ds_array.push_back(false);
                            pshape->tr_permutaton = new_tr_permutation;
                            pshape->tr_shape_id = new_tr_shape_id;
                            pshape->width = old_pshape->width + current_tr->num_finger + 1;
                            pshape->ds_array = new_ds_array;
                            new_partial_placement.push_back(pshape);
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
        // std::cout << "placement_cand.size(): " << placement_cand.size() << std::endl;
    }

    // print solution
    // std::cout << "min_cell_width: " << min_cell_width << std::endl;
    // std::cout << "number of cell: " << placement_cand.size() << std::endl;
    for (auto pshape: placement_cand) {
        // for (int i = 0; i < pshape->tr_permutaton.size(); i++) {
        //     Transistor* tr_p = pshape->tr_permutaton[i];
        //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
        //     for (int k = 0; k < tr_p->num_finger; k++) {
        //         std::cout << tr_p->name << " ";
        //     }
        // }
        // std::cout << std::endl;
        //print shape idx
        // for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
        //     Transistor* tr_p = pshape->tr_permutaton[i];
        //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
        //         for (int k = 0; k < tr_p->num_finger; k++) {
        //             std::cout << pshape->tr_shape_id[i] << " ";
        //         }
        // }
        // std::cout << std::endl;
        //print active
        // for (int i = 0; i < pshape->tr_shape_id.size(); i++) {
        //     Transistor* tr_p = pshape->tr_permutaton[i];
        //     // auto config = single_row_configs[tr_p][pshape->tr_shape_id[i]];
        //     if (pshape->ds_array[i] == false) {
        //         std::cout << " dummy_gate ";
        //     } 
        //     for (int k = 0; k < tr_p->num_finger; k++) {
        //         if (k % 2 == 0) {
        //             if (pshape->tr_shape_id[i] == 0) {
        //                 std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
        //             }
        //             else {
        //                 std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
        //             }
        //         }
        //         else {
        //             if (pshape->tr_shape_id[i] == 0) {
        //                 std::cout << std::left << std::setw(7) << tr_p->source->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->drain->name << " ";
        //             }
        //             else {
        //                 std::cout << std::left << std::setw(7 )<< tr_p->drain->name << " " << std::left << std::setw(7) << tr_p->gate->name << " " << std::left << std::setw(7) << tr_p->source->name << " ";
        //             }
        //         }
        //     }
        // }
        // std::cout << std::endl;
    }
    return placement_cand;
}

CFET::Signal* CFET::simple_get_right_active(Transistor* tr, int shape_idx) {
    return (tr->num_finger % 2 == 1) ? (shape_idx == 0) ? tr->source : tr->drain : (shape_idx == 0) ? tr->drain : tr->source;
}

CFET::Signal* CFET::simple_get_left_active(Transistor* tr, int shape_idx) {
    return (shape_idx == 0) ? tr->drain : tr->source;
}