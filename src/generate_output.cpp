#include <omp.h>
#include <sys/stat.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "cfet.h"
#include "z3++.h"

namespace fs = std::filesystem;

// 如果不存在就創建資料夾
inline bool createFolder(const std::string& name) { return fs::create_directory(name); }

// --------------------------------------------------------------------------------
// 單列分檔輸出
// --------------------------------------------------------------------------------
void generate_plmt(std::vector<Pshape*> pshape_vec) {
    const std::string folderName = "results";
    if (fs::exists(folderName)) {
        fs::remove_all(folderName);
        std::cout << "資料夾 " << folderName << " 已刪除。\n";
    } else {
        std::cout << "資料夾 " << folderName << " 不存在。\n";
    }
    if (!createFolder(folderName)) {
        std::cerr << "無法創建資料夾 " << folderName << "，可能已存在。\n";
    } else {
        std::cout << "資料夾 " << folderName << " 已成功創建！\n";
    }

    for (size_t i = 0; i < pshape_vec.size(); ++i) {
        Pshape* pshape = pshape_vec[i];
        std::string fileName = folderName + "/output_" + std::to_string(i) + ".plmt";
        std::ofstream ofs(fileName);
        if (!ofs) {
            std::cerr << "無法創建檔案 " << fileName << "\n";
            continue;
        }

        // NAME
        ofs << "<NAME> " << i << "\n";

        // OUTPUT
        ofs << "<OUTPUT> ";
        for (auto* sig : outputs) {
            for (auto* tr : pshape->multirow_tr_permutation_down[0]) {
                if (!tr) continue;
                if (tr->drain == sig || tr->source == sig) {
                    ofs << sig->name << " ";
                    break;
                }
            }
        }
        ofs << "\n<POWER> VDD\n<GROUND> VSS\n";

        // INPUT
        ofs << "<INPUT> ";
        for (auto* sig : inputs) {
            for (auto* tr : pshape->multirow_tr_permutation_down[0]) {
                if (!tr) continue;
                if (tr->gate == sig) {
                    ofs << sig->name << " ";
                    break;
                }
            }
        }
        ofs << "\n";

        // PMOS sizes
        ofs << "<PMOS> " << pshape->width << "\n";
        for (size_t r = 0; r < pshape->height; ++r) {
            ofs << "( ";
            for (size_t c = 0; c < pshape->width; ++c) {
                auto* tr = pshape->multirow_tr_permutation_down[r][c];
                int shape = pshape->multirow_tr_shape_down[r][c];
                int w = tr ? (tr->width / tr->num_finger) : 0;
                if (shape == 2) w = 81;
                ofs << w << " ";
            }
            ofs << ")\n";
        }

        // PMOS connectivity
        for (size_t r = 0; r < pshape->height; ++r) {
            size_t row_width = pshape->width;
            for (size_t c = 0; c < row_width; ++c) {
                auto* tr = pshape->multirow_tr_permutation_down[r][c];
                int shape = pshape->multirow_tr_shape_down[r][c];
                if (shape == 0) {
                    ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name << " ";
                } else if (shape == 1) {
                    ofs << tr->source->name << " <" << tr->gate->name << "> " << tr->drain->name << " ";
                } else if (shape == 2 && c > 0 && c + 1 < row_width) {
                    auto* pre = pshape->multirow_tr_permutation_down[r][c - 1];
                    auto* post = pshape->multirow_tr_permutation_down[r][c + 1];
                    std::string left = (pshape->multirow_tr_shape_down[r][c - 1] == 0 ? pre->source->name : pre->drain->name);
                    std::string right = (pshape->multirow_tr_shape_down[r][c + 1] == 0 ? post->drain->name : post->source->name);
                    ofs << left << " <VDD> " << right << " ";
                } else {
                    ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name << " ";
                }
            }
            ofs << "\n";
        }
        ofs << "\n";

        // NMOS sizes
        ofs << "<NMOS> " << pshape->width << "\n";
        for (size_t r = 0; r < pshape->height; ++r) {
            ofs << "( ";
            for (size_t c = 0; c < pshape->width; ++c) {
                auto* tr = pshape->multirow_tr_permutation_up[r][c];
                int shape = pshape->multirow_tr_shape_up[r][c];
                int w = tr ? (tr->width / tr->num_finger) : 0;
                if (shape == 2) w = 81;
                ofs << w << " ";
            }
            ofs << ")\n";
        }

        // NMOS connectivity
        for (size_t r = 0; r < pshape->height; ++r) {
            size_t row_width = pshape->width;
            for (size_t c = 0; c < row_width; ++c) {
                auto* tr = pshape->multirow_tr_permutation_up[r][c];
                int shape = pshape->multirow_tr_shape_up[r][c];
                if (shape == 0) {
                    ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name << " ";
                } else if (shape == 1) {
                    ofs << tr->source->name << " <" << tr->gate->name << "> " << tr->drain->name << " ";
                } else if (shape == 2 && c > 0 && c + 1 < row_width) {
                    auto* pre = pshape->multirow_tr_permutation_up[r][c - 1];
                    auto* post = pshape->multirow_tr_permutation_up[r][c + 1];
                    std::string left = (pshape->multirow_tr_shape_up[r][c - 1] == 0 ? pre->source->name : pre->drain->name);
                    std::string right = (pshape->multirow_tr_shape_up[r][c + 1] == 0 ? post->source->name : post->drain->name);
                    ofs << left << " <VSS> " << right << " ";
                } else {
                    ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name << " ";
                }
            }
            ofs << "\n";
        }

        ofs.close();
        std::cout << "內容已寫入檔案 " << fileName << "\n";
    }
}

void generate_output(Pshape* pshape) {
    bool via_rule_satisfied = pshape->satisfy_via_rule();
    pshape->generate_multirow_plmt();
}