#include <omp.h>

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "cfet.h"

int main(int argc, char **argv) {
    std::cout << "Hello, World!" << std::endl;

    assert(argc == 2);

    // Open the input file for reading
    std::string filename = argv[1];
    std::cout << filename << std::endl;
    // std::ifstream fin(argv[1]);
    // Check if the input file was opened successfully
    // if (!fin) {
    //     std::cerr << "Error: Could not open input file " << argv[1] << "\n";
    //     return 1;
    // }

    auto start = std::chrono::high_resolution_clock::now();
    int num_cores = omp_get_num_procs();
    std::cout << "Available CPU cores: " << num_cores << std::endl;
    omp_set_num_threads(1);
    parse_input(filename);
    new_tr_pairing();
    std::cout << "[folding_shape_generation]" << std::endl;
    folding_shape_generation();
    std::cout << "[placement]" << std::endl;
    Pshape *pshape = placement();
    Pshape *refined_pshape = detailed_placement(pshape);
    refined_pshape->generate_multirow_plmt();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken : " << elapsed.count() << " seconds" << std::endl;
    return 0;
}
