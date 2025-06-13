#include <omp.h>
#include <sys/stat.h>  // Linux/Unix 系統用於創建資料夾

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
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
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cfet.h"
#include "z3++.h"

int calculate_hpml(std::vector<Pshape *> single_row_vec) {
    int hpml = 0;
    std::unordered_map<Signal *, int> min_x;
    std::unordered_map<Signal *, int> max_x;
    std::unordered_map<Signal *, int> min_y;
    std::unordered_map<Signal *, int> max_y;
    std::cout << std::endl;
    for (auto pair : signals) {
        Signal *sig = pair.second;
        if (sig) {
            min_x[sig] = single_row_vec[0]->multirow_signal_permutation_down[0].size();
            max_x[sig] = 0;
            min_y[sig] = single_row_vec.size();
            max_y[sig] = 0;
        }
    }
    for (int i = 0; i < single_row_vec.size(); i++) {
        for (int j = 0; j < single_row_vec[i]->multirow_signal_permutation_up[0].size(); j++) {
            Signal *sig_up = single_row_vec[i]->multirow_signal_permutation_up[0][j];
            Signal *sig_down = single_row_vec[i]->multirow_signal_permutation_down[0][j];
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
            hpml += max_x[sig] - min_x[sig] + max_y[sig] - min_y[sig];
        }
    }
    std::cout << "hpml: " << hpml << std::endl;
    return hpml;
}

int inter_row_signal_count(std::vector<Pshape *> pshape_vec) {
    std::set<Signal *> signal_set;
    std::set<Signal *> interrow_signal_set;
    for (int i = 0; i < pshape_vec.size(); i++) {
        Pshape *pshape = pshape_vec[i];
        for (int j = 0; j < pshape->multirow_tr_permutation_down[0].size(); j++) {
            Transistor *tr_p = pshape->multirow_tr_permutation_down[0][j];
            Transistor *tr_n = pshape->multirow_tr_permutation_up[0][j];
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
        for (int i = 0; i < pshape_vec.size(); i++) {
            Pshape *pshape = pshape_vec[i];
            for (int j = 0; j < pshape->multirow_tr_permutation_down[0].size(); j++) {
                Transistor *tr_p = pshape->multirow_tr_permutation_down[0][j];
                Transistor *tr_n = pshape->multirow_tr_permutation_up[0][j];
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