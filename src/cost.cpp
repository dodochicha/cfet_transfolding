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

int Pshape::calculate_hpml() {
    int hpml = 0;
    std::unordered_map<Signal *, int> min_x;
    std::unordered_map<Signal *, int> max_x;
    std::unordered_map<Signal *, int> min_y;
    std::unordered_map<Signal *, int> max_y;
    const int rows = multirow_signal_permutation_up.size();
    const int cols = multirow_signal_permutation_up[0].size();
    std::cout << std::endl;
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
            std::cout << sig->name << " [" << max_x[sig] - min_x[sig] + max_y[sig] - min_y[sig] << "] " << max_x[sig] << " " << min_x[sig] << " " << max_y[sig]
                      << " " << min_y[sig] << std::endl;
            hpml += max_x[sig] - min_x[sig] + max_y[sig] - min_y[sig];
        }
    }
    std::cout << "hpml: " << hpml << std::endl;
    return hpml;
}