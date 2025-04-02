#include <cuda_runtime.h>
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
    placement();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken : " << elapsed.count() << " seconds" << std::endl;
    return 0;
}

// #include <algorithm>
// #include <iostream>
// #include <set>
// #include <vector>

// void findIntersectingElements(const std::vector<std::set<int>>& sets) {
//     // 用於儲存每個集合中有交集的元素
//     std::vector<std::set<int>> intersectingElements(sets.size());

//     for (size_t i = 0; i < sets.size(); ++i) {
//         for (size_t j = 0; j < sets.size(); ++j) {
//             if (i != j) {
//                 // 計算集合 i 和集合 j 的交集
//                 std::set<int> intersection;
//                 std::set_intersection(sets[i].begin(), sets[i].end(), sets[j].begin(), sets[j].end(), std::inserter(intersection, intersection.begin()));

//                 // 將交集元素加入到集合 i 的交集結果中
//                 intersectingElements[i].insert(intersection.begin(), intersection.end());
//             }
//         }
//     }

//     // 輸出每個集合中有交集的元素
//     for (size_t i = 0; i < intersectingElements.size(); ++i) {
//         std::cout << "Set " << i + 1 << " intersecting elements: ";
//         if (intersectingElements[i].empty()) {
//             std::cout << "None";
//         } else {
//             for (int elem : intersectingElements[i]) {
//                 std::cout << elem << " ";
//             }
//         }
//         std::cout << std::endl;
//     }
// }

// int main() {
//     // 定義 4 個集合
//     std::vector<std::set<int>> sets = {{1, 2, 3, 4}, {3, 4, 5, 6}, {6, 7, 8, 9}, {2, 4, 6, 8}};

//     // 找出每個集合中與其他集合有交集的元素
//     findIntersectingElements(sets);

//     return 0;
// }
