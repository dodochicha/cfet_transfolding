#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <regex>
#include <sstream>
#include <unordered_map>
// #include <vector>

#include "cfet.h"

void parse_input(std::string file) {
    size_t last_slash = file.find_last_of("/\\");
    size_t last_dot = file.find_last_of('.');

    // 取出檔名（不含路徑與副檔名）
    cell_name = file.substr(last_slash + 1, last_dot - last_slash - 1);

    std::ifstream stream(file);
    if (!stream) {
        std::cerr << "Error: Could not open input file " << file << "\n";
        return;
    }
    std::string token;
    std::string line;
    stream >> token;
    stream >> token;
    std::getline(stream, line);
    std::istringstream iss(line);
    std::string temp;
    while (iss >> temp) {
        Signal* sig = new Signal();
        sig->name = temp;
        sig->is_io = 1;
        sig->is_io_pins = true;
        signals.insert(std::make_pair(temp, sig));
        io_pins.push_back(temp);
    }

    while (std::getline(stream, line)) {
        std::istringstream iss(line);
        std::string drain, gate, source, body, mos_type;
        std::string length, width;
        std::string nfin;
        std::string token;
        Transistor* tr = new Transistor();
        if (iss >> token) {
            if (token == ".ENDS") {
                // std::cout << "end" << std::endl;
                break;
            } else {
                tr->name = new char[token.size() + 1];
                std::strcpy(tr->name, token.c_str());
            }
        }

        iss >> drain >> gate >> source >> body >> mos_type >> width >> length >> nfin;
        // std::cout << drain << " " << gate << " " << source << std::endl;

        auto it_d = signals.find(drain);
        if (it_d != signals.end()) {
            Signal* D = it_d->second;
            tr->drain = D;
        } else {
            Signal* D = new Signal();
            D->name = drain;
            signals.insert(std::make_pair(drain, D));
            tr->drain = D;
        }

        auto it_g = signals.find(gate);
        if (it_g != signals.end()) {
            Signal* D = it_g->second;
            tr->gate = D;
        } else {
            Signal* D = new Signal();
            D->name = gate;
            signals.insert(std::make_pair(gate, D));
            tr->gate = D;
        }

        auto it_s = signals.find(source);
        if (it_s != signals.end()) {
            Signal* D = it_s->second;
            tr->source = D;
        } else {
            Signal* D = new Signal();
            D->name = source;
            signals.insert(std::make_pair(source, D));
            tr->source = D;
        }

        auto it_b = signals.find(body);
        if (it_b != signals.end()) {
            Signal* D = it_b->second;
            tr->body = D;
        } else {
            Signal* D = new Signal();
            D->name = body;
            signals.insert(std::make_pair(body, D));
            tr->body = D;
        }

        std::regex re("([0-9]+\\.[0-9]+)");  // 匹配浮点数的正则表达式
        std::regex re_int("([0-9]+)");       // 匹配浮点数的正则表达式
        std::smatch match;
        if (std::regex_search(width, match, re)) {
            // 输出匹配的数字部分
            std::string matched_str = match[1].str();
            double width_value = std::stod(matched_str);
            tr->width = width_value;
        } else if (std::regex_search(width, match, re_int)) {
            // 输出匹配的数字部分
            std::string matched_str = match[1].str();
            double width_value = std::stod(matched_str);
            tr->width = width_value;
        }

        if (std::regex_search(length, match, re)) {
            // 输出匹配的数字部分
            // std::cout << match[1] << std::endl;
            std::string matched_str = match[1].str();
            double length_value = std::stod(matched_str);
            tr->length = length_value;
        } else if (std::regex_search(length, match, re_int)) {
            // 输出匹配的数字部分
            // std::cout << match[1] << std::endl;
            std::string matched_str = match[1].str();
            double length_value = std::stod(matched_str);
            tr->length = length_value;
        }

        if (std::regex_search(nfin, match, re_int)) {
            // 输出匹配的数字部分
            // std::cout << match[1] << std::endl;
            std::string matched_str = match[1].str();
            int nfin_value = std::stoi(matched_str);
            tr->nfin = nfin_value;
        }

        if (mos_type[0] == 'p') {
            tr->type = MosType::PMOS;
        } else {
            tr->type = MosType::NMOS;
        }
        trs.push_back(tr);
    }

    for (Transistor* tr : trs) {
        // std::cout << tr->name << " " << tr->width << std::endl;
        tr_dict.insert(std::make_pair(tr->name, tr));
        if (tr->type == MosType::PMOS) {
            pmos.push_back(tr);
        } else {
            nmos.push_back(tr);
        }
    }

    // num_finger
    for (Transistor* p : pmos) {
        p->num_finger = (p->width - 0.1) / max_cfet_width + 1;
        std::cout << p->name << " num_finger: " << p->num_finger << std::endl;
        // std::cout << p->drain->name << " " << p->gate->name << " " << p->source->name << std::endl;
    }
    for (Transistor* n : nmos) {
        n->num_finger = (n->width - 0.1) / max_cfet_width + 1;
        std::cout << n->name << " num_finger: " << n->num_finger << std::endl;
        // std::cout << n->drain->name << " " << n->gate->name << " " << n->source->name << std::endl;
    }
    std::cout << "#tr/#net: " << trs.size() << "/" << signals.size() << std::endl;
    // assign tr id
    for (int i = 0; i < pmos.size(); i++) {
        pmos[i]->id = i;
    }

    // io_pins
    for (int i = 0; i < io_pins.size(); i++) {
        Signal* sig = signals[io_pins[i]];
        sig->is_io_pins = true;
        if (sig->name == "VDD" || sig->name == "VSS") {
            continue;
        }
        for (auto tr : pmos) {
            if (sig == tr->drain || sig == tr->source) {
                outputs.insert(sig);
                break;
            } else if (sig == tr->gate) {
                inputs.insert(sig);
            }
        }
    }
}
