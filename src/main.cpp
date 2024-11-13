#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include <map>
#include "z3++.h"
#include "cfet.h"

using namespace z3;

int main(int argc, char* argv[]) {
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

    std::map<std::string, float> design_rule = {
        {"max_cfet_width", 81.0},
        {"diffusion_break_constraint", 1},
        {"max_placement_size", 100000},
        {"max_allowable_cell_height", 1},
        {"relaxation_parameter", 0}
    };
    CFET solver(design_rule);
    solver.parse_input(fin);
    // solver.tr_pairing();
    solver.new_tr_pairing();
    solver.folding_shape_generation();
    // solver.placement_single_row_search_tree();
    solver.placement_multi_row_search_tree();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken : " << elapsed.count() << " seconds" << std::endl;
    return 0;
}
