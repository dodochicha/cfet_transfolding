#include <omp.h>
#include <sys/stat.h>

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
#include <unordered_map>
#include <utility>
#include <vector>

#include "cfet.h"

Pshape* detailed_placement(Pshape* pshape) {
    const int alpha = 25;
    const int beta = 1;
    const int gamma = 1;
    const int rows = pshape->multirow_tr_permutation_up.size();
    const int cols = pshape->multirow_tr_permutation_up[0].size();
    int cost = pshape->cost(true);
    int best_cost = cost;
    std::cout << "[initial] cost: " << cost << std::endl;
    Pshape* best_pshape = pshape->copy();
    int improved_y;
    int improved_x;

    for (int i = 0; i < 100; i++) {
        std::cout << "[epoch " << i << "]" << std::endl;
        std::vector<std::pair<int, int>> most_improved_grid = pshape->choose_most_improved_target();
        bool improved = false;
        for (int j = 0; j < most_improved_grid.size(); j++) {
            // std::cout << "most_improved_grid: " << most_improved_grid[j].first << " " << most_improved_grid[j].second << std::endl;

            improved_y = most_improved_grid[j].first;
            if (most_improved_grid[j].second % 2 == 1) {
                improved_x = (most_improved_grid[j].second - 1) / 2;
            } else if (pshape->multirow_signal_permutation_up[most_improved_grid[j].first][most_improved_grid[j].second - 1] == nullptr) {
                improved_x = std::max(0, (most_improved_grid[j].second) / 2);
            } else {
                improved_x = std::max(0, (most_improved_grid[j].second - 2) / 2);
            }
            // std::cout << "improved grid: " << improved_y << " " << improved_x << std::endl;
            pshape->improve(improved_y, improved_x);
            pshape->allign();
            cost = pshape->cost();
            if (cost < best_cost) {
                best_cost = cost;
                std::cout << "[final result] cost: " << cost << std::endl;
                pshape->cost(true);
                improved = true;
                // if (pshape->satisfy_via_rule(false)) {
                //     best_pshape = pshape->copy();
                //     std::cout << "SAVE RESULT!" << std::endl;
                // }
                best_pshape = pshape->copy();
                break;
            }
        }
        if (improved == false) {
            break;
        }
    }

    bool via_rule_satisfied = best_pshape->satisfy_via_rule();
    assert(via_rule_satisfied);
    if (via_rule_satisfied == false) {
        std::cout << "fail placement" << std::endl;
    }

    best_pshape->print_pshape();

    return best_pshape;
}