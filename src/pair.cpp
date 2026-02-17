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
    std::cout << "nets: " << std::endl;
    std::vector<Signal*> nets;
    for (auto pair : signals) {
        if (pair.first != "VDD" && pair.first != "VSS") {
            std::cout << pair.second->name << std::endl;
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
    std::vector<Transistor*> _pmos;
    std::vector<Transistor*> _nmos;
    _pmos = pmos;
    _nmos = nmos;
    for (auto group : compound_gates) {
        // find p-network and n-network
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
        stable_matching(p_network, n_network);
    }
    // print pairs
    std::cout << "pairs: " << std::endl;
    std::unordered_map<std::string, int> num_mm;
    for (auto pair : tr_pairs) {
        std::cout << pair.first->name << " " << pair.second->name << std::endl;
        num_mm[pair.first->name]++;
        num_mm[pair.second->name]++;
    }

    std::vector<Transistor*> remaining_pmos;
    std::vector<Transistor*> remaining_nmos;
    for (int i = 0; i < pmos.size(); i++) {
        if (num_mm[pmos[i]->name] == 0) {
            remaining_pmos.push_back(pmos[i]);
        }
        if (num_mm[nmos[i]->name] == 0) {
            remaining_nmos.push_back(nmos[i]);
        }
    }
    stable_matching(remaining_pmos, remaining_nmos);
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

void stable_matching(std::vector<Transistor*> pmos_set, std::vector<Transistor*> nmos_set) {
    const int N = pmos_set.size();
    std::vector<std::vector<int>> costs(N, std::vector<int>(N, 0));        // [p][n]
    std::vector<std::vector<int>> prefNmos(N, std::vector<int>(N, 0));     // [n][p]
    std::vector<std::vector<int>> prefPmos(N, std::vector<int>(N, 0));     // [p][n]
    std::vector<std::vector<int>> nmosRanking(N, std::vector<int>(N, 0));  // [p][n]
    std::queue<int> freeNmos;
    std::vector<bool> nmosFree(N, true);
    std::vector<int> nextProposal(N, 0);  // nmos[i] propose to pmos
    std::vector<int> pmosPartner(N, -1);
    for (int i = 0; i < pmos_set.size(); i++) {
        Transistor* tr_p = pmos_set[i];
        for (int j = 0; j < nmos_set.size(); j++) {
            Transistor* tr_n = nmos_set[j];
            bool same_width = tr_p->width == tr_n->width;
            bool same_d = tr_p->drain == tr_n->drain;
            bool same_g = tr_p->gate == tr_n->gate;
            bool same_s = tr_p->source == tr_n->source;
            int routablility = 0;
            if (tr_p->drain->name == "VDD" && tr_n->drain->name != "VSS" || tr_p->drain->name != "VDD" && tr_n->drain->name == "VSS" ||
                tr_p->drain->name == tr_n->drain->name) {
                routablility += 1;
            } else if (tr_p->drain->name != "VDD" && tr_n->drain->name != "VSS" && tr_p != tr_n) {
                routablility += 2;
            }
            if (tr_p->gate->name == "VDD" && tr_n->gate->name != "VSS" || tr_p->gate->name != "VDD" && tr_n->gate->name == "VSS" ||
                tr_p->gate->name == tr_n->gate->name) {
                routablility += 1;
            } else if (tr_p->gate->name != "VDD" && tr_n->gate->name != "VSS" && tr_p != tr_n) {
                routablility += 2;
            }
            if (tr_p->source->name == "VDD" && tr_n->source->name != "VSS" || tr_p->source->name != "VDD" && tr_n->source->name == "VSS" ||
                tr_p->source->name == tr_n->source->name) {
                routablility += 1;
            } else if (tr_p->source->name != "VDD" && tr_n->source->name != "VSS" && tr_p != tr_n) {
                routablility += 2;
            }
            int cost = 2 * same_width + (same_d + same_g + same_s) - routablility;
            costs[i][j] = cost;
            std::cout << tr_p->name << " " << tr_n->name << " " << cost << std::endl;
        }
    }

    for (int i = 0; i < N; i++) {
        std::vector<int> id_arr;
        for (int j = 0; j < N; j++) {
            id_arr.push_back(j);
        }
        std::sort(id_arr.begin(), id_arr.end(), [&costs, &i](int a, int b) { return costs[i][a] > costs[i][b]; });
        prefNmos[i] = id_arr;
    }

    for (int i = 0; i < N; i++) {
        std::vector<int> id_arr;
        for (int j = 0; j < N; j++) {
            id_arr.push_back(j);
        }
        std::sort(id_arr.begin(), id_arr.end(), [&costs, &i](int a, int b) { return costs[a][i] > costs[b][i]; });
        prefPmos[i] = id_arr;
    }

    for (int i = 0; i < N; i++) {
        freeNmos.push(i);
    }

    for (int i = 0; i < prefPmos.size(); i++) {
        for (int j = 0; j < prefPmos[i].size(); j++) {
            std::cout << nmos_set[i]->name << " pref " << pmos_set[prefPmos[i][j]]->name << std::endl;
        }
    }

    for (int i = 0; i < prefPmos.size(); i++) {
        for (int j = 0; j < prefPmos[i].size(); j++) {
            std::cout << pmos_set[i]->name << " pref " << nmos_set[prefNmos[i][j]]->name << std::endl;
        }
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            nmosRanking[i][prefNmos[i][j]] = j;
        }
    }

    while (!freeNmos.empty()) {
        int nid = freeNmos.front();
        freeNmos.pop();
        int pid = prefPmos[nid][nextProposal[nid]];
        int current = pmosPartner[pid];
        std::cout << nmos_set[nid]->name << " propose #" << nextProposal[nid] << std::endl;

        if (current == -1) {
            pmosPartner[pid] = nid;
            std::cout << nmos_set[pid]->name << " match " << pmos_set[nid]->name << std::endl;
            nmosFree[nid] = false;
        } else if (nmosRanking[pid][nid] < nmosRanking[pid][current]) {
            // } else if (prefNmos[pid][nid] < prefNmos[pid][current]) {
            pmosPartner[pid] = nid;
            std::cout << nmos_set[pid]->name << " match " << pmos_set[nid]->name << std::endl;
            nmosFree[nid] = false;
            nmosFree[current] = true;
            freeNmos.push(current);
        } else {
            freeNmos.push(nid);
        }
        nextProposal[nid]++;
    }

    std::cout << "[matching result]" << std::endl;
    for (int i = 0; i < N; i++) {
        std::cout << pmos_set[i]->name << " <-> " << nmos_set[pmosPartner[i]]->name << std::endl;
        tr_pairs[pmos_set[i]] = nmos_set[pmosPartner[i]];
    }
}