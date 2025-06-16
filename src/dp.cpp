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
#include <utility>
#include <vector>

#include "cfet.h"

namespace fs = std::filesystem;

Pshape *detailed_placement(std::vector<Pshape *> single_row_vec) {
    Pshape *multirow_pshape = new Pshape();
    int initial_width = single_row_vec[0]->multirow_tr_permutation_up[0].size();
    multirow_pshape->multirow_tr_permutation_up.resize(single_row_vec.size());
    multirow_pshape->multirow_tr_permutation_down.resize(single_row_vec.size());
    multirow_pshape->multirow_signal_permutation_up.resize(single_row_vec.size());
    multirow_pshape->multirow_signal_permutation_down.resize(single_row_vec.size());
    multirow_pshape->multirow_tr_shape_up.resize(single_row_vec.size());
    multirow_pshape->multirow_tr_shape_down.resize(single_row_vec.size());
    for (int i = 0; i < single_row_vec.size(); i++) {
        multirow_pshape->multirow_tr_permutation_up[i] = single_row_vec[i]->multirow_tr_permutation_up[0];
        multirow_pshape->multirow_tr_permutation_down[i] = single_row_vec[i]->multirow_tr_permutation_down[0];
        multirow_pshape->multirow_signal_permutation_up[i] = single_row_vec[i]->multirow_signal_permutation_up[0];
        multirow_pshape->multirow_signal_permutation_down[i] = single_row_vec[i]->multirow_signal_permutation_down[0];
        multirow_pshape->multirow_tr_shape_up[i] = single_row_vec[i]->multirow_tr_shape_up[0];
        multirow_pshape->multirow_tr_shape_down[i] = single_row_vec[i]->multirow_tr_shape_down[0];
    }
    int inter_row_signal_num = multirow_pshape->inter_row_signal_count();
    int hpml = multirow_pshape->calculate_hpml();
    multirow_pshape->allign();
    std::cout << "[move]" << std::endl;
    multirow_pshape->move_tr_to_left(1, 0);
    inter_row_signal_num = multirow_pshape->inter_row_signal_count();
    hpml = multirow_pshape->calculate_hpml();
    std::cout << "[move]" << std::endl;
    multirow_pshape->move_tr_to_left(0, 0);
    inter_row_signal_num = multirow_pshape->inter_row_signal_count();
    hpml = multirow_pshape->calculate_hpml();
    return multirow_pshape;
}