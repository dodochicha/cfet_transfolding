#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include <regex>
#include <iomanip>

#include "cfet.h"

void CFET::parse_input(std::istream& stream) {
    std::string token;
    std::string line;
    std::vector<std::string> io_pins;  
    std::string io_pin;
//   stream >> token;
//   std::cout << token << std::endl;
//   stream >> token;
//   std::cout << token << std::endl;
    std::getline(stream, line);
    std::istringstream iss(line); 


    int word_num = 0;
    while (iss >> io_pin) {
        if (word_num >= 2) {
            io_pins.push_back(io_pin);
        }
        word_num++;
    }
    
    // for (const auto& io_pin : io_pins) {
    //     std::cout << io_pin << std::endl;
    // }

    while (std::getline(stream, line)) {
        std::istringstream iss(line); 
        std::string drain, gate, source, body, mos_type;
        std::string length, width;
        std::string nfin;
        Transistor* tr = new Transistor();
        if (iss >> token) {
            if (token == ".ENDS") break;
            else {
                tr->name = token;
            }
        }
        iss >> drain >> gate >> source >> body >> mos_type >> width >> length >> nfin;
        
        auto it_d = signals.find(drain);
        if (it_d != signals.end()) {
            Signal* D = it_d->second;
            tr->drain = D;
        }
        else {
            Signal* D = new Signal();
            D->name = drain;
            signals.insert(std::make_pair(drain, D));
            tr->drain = D;
        }
        
        auto it_g = signals.find(gate);
        if (it_g != signals.end()) {
            Signal* D = it_g->second;
            tr->gate = D;
        }
        else {
            Signal* D = new Signal();
            D->name = gate;
            signals.insert(std::make_pair(gate, D));
            tr->gate = D;
        }
        
        auto it_s = signals.find(source);
        if (it_s != signals.end()) {
            Signal* D = it_s->second;
            tr->source = D;
        }
        else {
            Signal* D = new Signal();
            D->name = source;
            signals.insert(std::make_pair(source, D));
            tr->source = D;
        }

        auto it_b = signals.find(body);
        if (it_b != signals.end()) {
            Signal* D = it_b->second;
            tr->body = D;
        }
        else {
            Signal* D = new Signal();
            D->name = body;
            signals.insert(std::make_pair(body, D));
            tr->body = D;
        }
        
        std::regex re("([0-9]+\\.[0-9]+)");  // 匹配浮点数的正则表达式
        std::regex re_int("([0-9]+)");  // 匹配浮点数的正则表达式
        std::smatch match;
        if (std::regex_search(width, match, re)) {
            // 输出匹配的数字部分
            std::string matched_str = match[1].str();
            double width_value = std::stod(matched_str);
            tr->width = width_value;
        }
        else if (std::regex_search(width, match, re_int)) {
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
        }
        else if (std::regex_search(length, match, re_int)) {
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
        }
        else {
            tr->type = MosType::NMOS;
        }
        trs.push_back(tr);
    }

    for (Transistor* tr: trs) {
        // std::cout << tr->name << " " << tr->width << std::endl;
        tr_dict.insert(std::make_pair(tr->name, tr));
        if (tr->type == MosType::PMOS) {
            pmos.push_back(tr);
        }
        else {
            nmos.push_back(tr);
        }
    }
    
    // num_finger
    for (Transistor* p: pmos) {
        p->num_finger = (p->width-0.1) / max_cfet_width + 1;
        std::cout << p->name << " num_finger: " << p->num_finger << std::endl;
        std::cout << p->drain->name << " " << p->gate->name << " " << p->source->name << std::endl;
    }
    for (Transistor* n: nmos) {
        n->num_finger = (n->width-0.1) / max_cfet_width + 1;
        std::cout << n->name << " num_finger: " << n->num_finger << std::endl;
        std::cout << n->drain->name << " " << n->gate->name << " " << n->source->name << std::endl;
    }
    std::cout << "#tr/#net: " << trs.size() << "/" << signals.size() << std::endl;
}

void CFET::print_layout(std::ostream& stream) {
    std::vector<std::string> signal_layout_n;
    std::vector<std::string> signal_layout_p;
    // for (int i = 0 ; i < pmos.size() ; i++) {
    //     stream << pmos[i]->name << " ";
    // }
    // for (int i = 0 ; i < ds_layout.size() ; i++) {
    //     stream << "\n";
    //     for (int j = 0 ; j < ds_layout[i].size() ; j++) {
    //         stream << ds_layout[i][j] << " ";
    //     }
    // }
    // stream << "\n";
    // stream << "\n";
    // for (int i = 0 ; i < tr_shape_idx_layout.size() ; i++) {
    //     stream << tr_shape_idx_layout[i] << " ";
    // }
    // stream << "\n";
    int config_size = single_row_configs[pmos[0]].size();
    for (int i = 0 ; i < pmos.size() ; i++) {
            // stream << "\n";
            // stream << "\n";
            // stream << tr_pairs[pmos[tr_layout[i]]]->name << "\n";
            // for (int k = 0 ; k < single_row_configs[pmos[tr_layout[i]]][tr_shape_idx_layout[tr_layout[i]]][0].size() ; k++) {
            //     stream << single_row_configs[pmos[tr_layout[i]]][tr_shape_idx_layout[tr_layout[i]]][0][k];
            // }
            // stream << "\n";
            for (int k = 0 ; k < single_row_configs[pmos[tr_layout[i]]][tr_shape_idx_layout[tr_layout[i]]][0].size() ; k++) {
                if (single_row_configs[pmos[tr_layout[i]]][tr_shape_idx_layout[tr_layout[i]]][0][k] == 0) {
                    // stream << tr_pairs[pmos[tr_layout[i]]]->drain->name << "/" << tr_pairs[pmos[tr_layout[i]]]->gate->name << "/" << tr_pairs[pmos[tr_layout[i]]]->source->name << " ";
                    signal_layout_n.push_back(tr_pairs[pmos[tr_layout[i]]]->drain->name);
                    signal_layout_n.push_back(tr_pairs[pmos[tr_layout[i]]]->gate->name);
                    signal_layout_n.push_back(tr_pairs[pmos[tr_layout[i]]]->source->name);
                }
                else {
                    // stream << tr_pairs[pmos[tr_layout[i]]]->source->name << "/" << tr_pairs[pmos[tr_layout[i]]]->gate->name << "/" << tr_pairs[pmos[tr_layout[i]]]->drain->name << " ";
                    signal_layout_n.push_back(tr_pairs[pmos[tr_layout[i]]]->source->name);
                    signal_layout_n.push_back(tr_pairs[pmos[tr_layout[i]]]->gate->name);
                    signal_layout_n.push_back(tr_pairs[pmos[tr_layout[i]]]->drain->name);
                }
            }
            // stream << "\n";
            // stream << pmos[tr_layout[i]]->name << "\n";
            for (int k = 0 ; k < single_row_configs[pmos[tr_layout[i]]][tr_shape_idx_layout[tr_layout[i]]][pmos[tr_layout[i]]->num_finger].size() ; k++) {
                // stream << single_row_configs[pmos[tr_layout[i]]][tr_shape_idx_layout[tr_layout[i]]][pmos[tr_layout[i]]->num_finger][k];
            }
            // stream << "\n";
            for (int k = 0 ; k < single_row_configs[pmos[tr_layout[i]]][tr_shape_idx_layout[tr_layout[i]]][pmos[tr_layout[i]]->num_finger].size() ; k++) {
                if (single_row_configs[pmos[tr_layout[i]]][tr_shape_idx_layout[tr_layout[i]]][pmos[tr_layout[i]]->num_finger][k] == 0) {
                    // stream << pmos[tr_layout[i]]->drain->name << "/"  << pmos[tr_layout[i]]->gate->name << "/" << pmos[tr_layout[i]]->source->name << " ";
                    signal_layout_p.push_back(pmos[tr_layout[i]]->drain->name);
                    signal_layout_p.push_back(pmos[tr_layout[i]]->gate->name);
                    signal_layout_p.push_back(pmos[tr_layout[i]]->source->name);
                }
                else {
                    // stream << pmos[tr_layout[i]]->source->name << "/" << pmos[tr_layout[i]]->gate->name << "/" << pmos[tr_layout[i]]->drain->name << " ";
                    signal_layout_p.push_back(pmos[tr_layout[i]]->source->name);
                    signal_layout_p.push_back(pmos[tr_layout[i]]->gate->name);
                    signal_layout_p.push_back(pmos[tr_layout[i]]->drain->name);
                }
            }
            if (i != pmos.size()-1) {
                if (ds_layout[tr_layout[i]][tr_layout[i+1]] == false) {
                    // std::cout << "dummy: " << i << std::endl;
                    signal_layout_n.push_back("dummy_gate");
                    signal_layout_p.push_back("dummy_gate");
                }
            }
    }
    stream << "cell width: " << width_layout + 2;
    stream << "\n";
    for (int i = 0 ; i < tr_layout.size() ; i++) {
        stream << tr_pairs[pmos[tr_layout[i]]]->name << " ";
    }
    stream << "\n";
    for (int i = 0 ; i < tr_layout.size() ; i++) {
        stream << pmos[tr_layout[i]]->name << " ";
    }
    stream << "\n";
    for (std::string sig: signal_layout_n) {
        stream << std::left << std::setw(7) << sig << "/";
    }
    stream << "\n";
    for (std::string sig: signal_layout_p) {
        stream << std::left << std::setw(7) << sig << "/";
    }

}