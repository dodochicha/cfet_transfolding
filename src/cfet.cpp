#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include <map>

#include "cfet.h"

CFET::CFET(const std::map<std::string, float>& attributes) {
    max_cfet_width = attributes.at("max_cfet_width");
    diffusion_break_constraint = attributes.at("diffusion_break_constraint");
    max_placement_size = attributes.at("max_placement_size");
    max_allowable_cell_height = attributes.at("max_allowable_cell_height");
    relaxation_parameter = attributes.at("relaxation_parameter");
}

CFET::Shape::Shape() {
    num_row = 0;
    num_finger = 0;
    id = 0;
}
CFET::Transistor::Transistor() {

}

CFET::Signal::Signal() {

}

CFET::Pshape::Pshape() {

}

CFET::Node::Node() {
    fill = 0;
}

CFET::Lambda::Lambda() {

}