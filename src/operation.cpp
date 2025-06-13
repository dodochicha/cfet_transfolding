#include <omp.h>
#include <sys/stat.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "cfet.h"
#include "z3++.h"

void Pshape::move_tr_to_left(int y, int x) {
    std::cout << "hello move_tr_to_left" << std::endl;
    Transistor *tr_up = multirow_tr_permutation_up[y][x];
    Transistor *tr_down = multirow_tr_permutation_down[y][x];
    Signal *drain_up = multirow_tr_permutation_up[y][x]->drain;
    Signal *source_up = multirow_tr_permutation_up[y][x]->source;
    Signal *drain_down = multirow_tr_permutation_down[y][x]->drain;
    Signal *source_down = multirow_tr_permutation_down[y][x]->source;
    for (int i = 0; i < multirow_tr_permutation_up.size(); i++) {
        auto pair = get_most_left_sig(i);
        Signal *sig_up = pair.first;
        Signal *sig_down = pair.second;
        bool sudu = (sig_up == drain_up);
        bool susu = (sig_up == source_up);
        bool sddd = (sig_down == drain_down);
        bool sdsd = (sig_down == source_down);
        if ((sudu || susu) && (sddd || sdsd)) {
            int shape_up = (sudu) ? 0 : 1;
            int shape_down = (sddd) ? 0 : 1;

            multirow_tr_permutation_up[i].push_back(tr_up);
            multirow_tr_permutation_down[i].push_back(tr_down);
            multirow_tr_shape_up[i].push_back(shape_up);
            multirow_tr_shape_down[i].push_back(shape_down);

            multirow_tr_permutation_up[y][x] = nullptr;
            multirow_tr_permutation_down[y][x] = nullptr;
            multirow_tr_shape_up[y][x] = 2;
            multirow_tr_shape_down[y][x] = 2;
            break;
        }
    }
    allign();
}