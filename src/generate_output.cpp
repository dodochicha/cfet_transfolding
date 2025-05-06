#include <omp.h>
#include <sys/stat.h>  // Linux/Unix 系統用於創建資料夾

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
// 多列輸出成 single‐block 的 .plmt
// --------------------------------------------------------------------------------
void generate_multirow_plmt(std::vector<Pshape*> pshape_vec, std::vector<std::vector<bool>> via_preassignment) {
    const std::string folderName = "results";
    if (fs::exists(folderName)) fs::remove_all(folderName);
    if (!createFolder(folderName)) {
        std::cerr << "無法創建資料夾 " << folderName << "\n";
    }

    // 計算 header 要用的最大欄數
    int max_cols = 0;
    for (auto* ps : pshape_vec) {
        max_cols = std::max(max_cols, ps->width);
    }

    std::string fileName = folderName + "/output.plmt";
    std::ofstream ofs(fileName);
    if (!ofs) {
        std::cerr << "無法開啟檔案 " << fileName << "\n";
        return;
    }

    size_t rows = pshape_vec.size();
    int cols = via_preassignment.empty() ? 0 : via_preassignment[0].size();

    // Header
    ofs << "<NAME> 0\n"
        << "<POWER> VDD\n"
        << "<GROUND> VSS\n"
        << "<OUTPUT> ";
    for (auto* sig : outputs) ofs << sig->name << " ";
    ofs << "\n<INPUT> ";
    for (auto* sig : inputs) ofs << sig->name << " ";
    ofs << "\n<ROWS> " << rows << " <COLS> " << max_cols * 2 + 1 << "\n\n";

    // PMOS block
    ofs << "<PMOS>\n";
    for (size_t r = 0; r < rows; ++r) {
        Pshape* ps = pshape_vec[r];
        size_t row_width = ps->width;
        for (size_t c = 0; c < row_width; ++c) {
            auto* tr = ps->multirow_tr_permutation_down[0][c];
            int shape = ps->multirow_tr_shape_down[0][c];
            if (shape == 0) {
                ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name;
            } else if (shape == 1) {
                ofs << tr->source->name << " <" << tr->gate->name << "> " << tr->drain->name;
            } else if (shape == 2 && c > 0 && c + 1 < row_width) {
                auto* pre = ps->multirow_tr_permutation_down[0][c - 1];
                auto* post = ps->multirow_tr_permutation_down[0][c + 1];
                std::string left = (ps->multirow_tr_shape_down[0][c - 1] == 0 ? pre->source->name : pre->drain->name);
                std::string right = (ps->multirow_tr_shape_down[0][c + 1] == 0 ? post->drain->name : post->source->name);
                ofs << left << " <VDD> " << right;
            } else {
                // fallback at edges
                ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name;
            }
            if (c + 1 < row_width) ofs << " ";
        }
        ofs << "\n";
    }
    ofs << "\n";

    // NMOS block
    ofs << "<NMOS>\n";
    for (size_t r = 0; r < rows; ++r) {
        Pshape* ps = pshape_vec[r];
        size_t row_width = ps->width;
        for (size_t c = 0; c < row_width; ++c) {
            auto* tr = ps->multirow_tr_permutation_up[0][c];
            int shape = ps->multirow_tr_shape_up[0][c];
            if (shape == 0) {
                ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name;
            } else if (shape == 1) {
                ofs << tr->source->name << " <" << tr->gate->name << "> " << tr->drain->name;
            } else if (shape == 2 && c > 0 && c + 1 < row_width) {
                auto* pre = ps->multirow_tr_permutation_up[0][c - 1];
                auto* post = ps->multirow_tr_permutation_up[0][c + 1];
                std::string left = (ps->multirow_tr_shape_up[0][c - 1] == 0 ? pre->source->name : pre->drain->name);
                std::string right = (ps->multirow_tr_shape_up[0][c + 1] == 0 ? post->source->name : post->drain->name);
                ofs << left << " <VSS> " << right;
            } else {
                // fallback at edges
                ofs << tr->drain->name << " <" << tr->gate->name << "> " << tr->source->name;
            }
            if (c + 1 < row_width) ofs << " ";
        }
        ofs << "\n";
    }
    ofs << "\n";

    // VIA Preassignment block
    ofs << "<VIA_PREASSIGNMENT>\n";
    for (int i = 0; i < rows * 4; ++i) {
        for (int j = 0; j < cols; ++j) {
            // 若 via_preassignment 尺寸不符，就輸出 0
            bool v = (i < (int)via_preassignment.size() && j < (int)via_preassignment[i].size()) ? via_preassignment[i][j] : false;
            ofs << (v ? '1' : '0') << ' ';
        }
        ofs << "\n";
    }
    ofs << "\n";

    ofs.close();
    std::cout << "Generated " << fileName << "\n";
}

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
