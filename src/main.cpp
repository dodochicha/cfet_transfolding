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

    assert(argc == 3);

    // Open the input file for reading
    std::ifstream fin(argv[1]);
    // Check if the input file was opened successfully
    if (!fin) {
        std::cerr << "Error: Could not open input file " << argv[1] << "\n";
        return 1;
    }

    // Open the output file for writing
    std::ofstream fout(argv[2]);
    // Check if the output file was opened successfully
    if (!fout) {
        std::cerr << "Error: Could not open output file " << argv[2] << "\n";
        return 1;
    }
    auto start = std::chrono::high_resolution_clock::now();
    int num_cores = omp_get_num_procs();
    std::cout << "Available CPU cores: " << num_cores << std::endl;
    omp_set_num_threads(16);
    parse_input(fin);
    new_tr_pairing();
    folding_shape_generation();
    std::vector<Pshape *> single_row_vec = placement();
    Pshape *pshape = detailed_placement(single_row_vec);
    generate_output(pshape);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken : " << elapsed.count() << " seconds" << std::endl;
    return 0;
}
