#ifndef CFET_H_
#define CFET_H_

#include <chrono>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

enum class MosType { PMOS, NMOS };

class Transistor;
class Signal;
class Shape;
class Pshape;
class Node;
class Lambda;
class Permutation;
class Group;
class Group_pair;

extern std::string cell_name;
extern std::vector<std::string> io_pins;
extern std::set<Signal*> outputs;
extern std::set<Signal*> inputs;
extern std::vector<Transistor*> trs;
extern std::vector<Transistor*> pmos;
extern std::vector<Transistor*> nmos;
extern std::unordered_map<std::string, Transistor*> tr_dict;
extern std::unordered_map<std::string, Signal*> signals;
extern std::unordered_map<Transistor*, Transistor*> tr_pairs;
extern std::unordered_map<Transistor*, std::unordered_map<Shape*, std::vector<Lambda*>>> phi;
extern std::unordered_map<Transistor*, std::unordered_map<Shape*, std::vector<Lambda*>>> phi_merged;
extern std::unordered_map<std::string, Shape*> CFETShapes;
extern std::unordered_map<Transistor*, std::vector<Lambda*>> multi_row_configs;
extern int tr_size_sum;
extern std::unordered_map<int, Lambda*> lambda_dict;
extern int* ds_dict;
extern int lamb_id;

extern int max_cfet_width;
extern int diffusion_break_constraint;
extern int max_placement_size;
extern int max_allowable_cell_height;
extern int num_nodes_parsed_to_gpu;
extern float expected_row_num;
extern float relaxation_parameter;
extern float aspect_ratio;

const double alpha = 25;    // area
const double beta = 1;      // hpml
const double gamma = 15;    // #inter-row signal
const double delta = 0.01;  // density

// void parse_input(std::istream& stream);
void parse_input(std::string file);
void new_tr_pairing();
void folding_shape_generation();
void generateBias(const std::vector<int>& row_bias, std::vector<int>& bias, size_t index, bool& found);
void stable_matching(std::vector<Transistor*> pmos_set, std::vector<Transistor*> nmos_set);
bool check_overlapped(Shape* big_shape, Shape* small_shape, int x, int y);
bool compareGroupPair(const Group_pair* a, const Group_pair* b);
bool compareRoutability(const Pshape* a, const Pshape* b);
bool merge_enable(Pshape* old_pshape, Lambda* new_lamb, Transistor* nmos, Transistor* pmos, int row, int col);

int hsp(Pshape* pshape);
int hcd(Pshape* pshape);
int check_connection(int row, int column, std::vector<bool> bool_vars);
int pairing_cost(Transistor* tr_n, Transistor* tr_p);
Pshape* merge(Pshape* old_pshape, Lambda* new_lamb, Transistor* nmos, Transistor* pmos, int row, int col);
Signal* get_right_active(Transistor* tr, int tr_shape_id);
Signal* get_left_active(Transistor* tr, int tr_shape_id);
Pshape* placement();
Pshape* placement_abutting(std::vector<Pshape*> pshape_vec);
Pshape* placement_merging(std::vector<Pshape*> pshape_vec);
Pshape* pshape_insert(Pshape* ori_pshape, Pshape* ins_pshape, int row, int col);
std::vector<Shape*> finger_slot_configuration(int finger);
std::vector<Pshape*> group_placement(std::vector<Transistor*> pmos_group);
std::vector<Pshape*> dfs_placement(Node* root_node, int target_area, std::vector<Transistor*> pmos_group);
std::vector<Pshape*> multirow_assignment(std::vector<Pshape*> pshape_vec);
Pshape* detailed_placement(Pshape* pshape);
Pshape* abut_on_top(Pshape* p1, Pshape* p2);
Pshape* abut_on_bottom(Pshape* p1, Pshape* p2);
Pshape* horizontal_flip(Pshape* p);
std::vector<std::vector<bool>> stack_feasibility(Shape* big_shape, Shape* small_shape);
std::vector<std::set<Signal*>> findIntersectingElements(const std::vector<std::set<Signal*>>& sets);
std::pair<std::vector<Node*>, std::vector<Pshape*>> bfs_placement(int target_area, std::vector<Transistor*> pmos_group);
std::pair<std::vector<Node*>, std::vector<Permutation*>> bfs_placement_singlerow(int target_area);

class Transistor {
   public:
    Transistor();

    // std::string name;
    char* name;

    Shape* single_row_shape;
    Signal* drain;
    Signal* gate;
    Signal* source;
    Signal* body;

    MosType type;

    double width;
    double length;
    int nfin;
    int num_finger;
    int* lamb_id;
    int lamb_size;
    int id;

    std::vector<Shape*> es;
    std::vector<Transistor*> neighbors;
};

class Shape {
   public:
    Shape();

    int num_row;
    int num_finger;
    int id;
    std::string name;
    std::vector<std::vector<bool>> config;
};

class Signal {
   public:
    Signal();

    std::string name;
    bool is_io = 0;  // 0: others, 1: io
    bool is_io_pins = false;
    int start = -1;
    int end = -1;
    int id;
};

class Pshape {
   public:
    Pshape();

    ~Pshape() {
        multirow_tr_shape_up.clear();
        multirow_tr_shape_up.shrink_to_fit();
        multirow_tr_shape_down.clear();
        multirow_tr_shape_down.shrink_to_fit();
        multirow_tr_permutation_up.clear();
        multirow_tr_permutation_up.shrink_to_fit();
        multirow_tr_permutation_down.clear();
        multirow_tr_permutation_down.shrink_to_fit();
    }

    // std::string name;
    // std::vector<Transistor*> tr_permutaton;
    // std::vector<int> tr_shape_id;
    // std::vector<bool> ds_array;
    // int width;
    std::string name;
    Transistor** tr_permutaton;
    int* tr_shape_id;
    int* ds_array;
    int width;
    int id;

    // multi-row
    std::vector<std::vector<int>> multirow_tr_shape_up;
    std::vector<std::vector<int>> multirow_tr_shape_down;
    std::vector<std::vector<Transistor*>> multirow_tr_permutation_up;
    std::vector<std::vector<Transistor*>> multirow_tr_permutation_down;
    std::vector<std::vector<Signal*>> multirow_signal_permutation_up;
    std::vector<std::vector<Signal*>> multirow_signal_permutation_down;
    std::vector<std::vector<Signal*>> m0_metal_fill;
    std::vector<int> most_right_idx;
    std::vector<std::vector<int>> available_track_case;
    std::vector<std::vector<Signal*>> via_preassignment;
    std::set<Signal*> interrow_signal_set;

    int multirow_area;
    int height;
    int multirow_macro_area;
    int top_width;
    int hsp;
    int hcd;
    int hpml;

    void print_pshape();
    std::pair<Signal*, Signal*> get_most_left_sig(int row);       // <sig_up, sig_down>
    std::pair<Signal*, Signal*> get_left_sig(int row, int col);   // <sig_up, sig_down>
    std::pair<Signal*, Signal*> get_right_sig(int row, int col);  // <sig_up, sig_down>
    void allign();
    void move_tr_to_left(int y, int x);
    void move_tr(int y1, int x1, int y2, int x2);
    void improve(int improved_y, int improved_x);
    void shift_row(int row);
    bool satisfy_via_rule(bool optimize = true);
    void calculate_available_track_case();
    void generate_multirow_plmt();
    int inter_row_signal_count(bool print = false);
    int calculate_hpml(bool print = false);
    int calculate_area(bool print = false);
    int calculate_metal_density(bool print = false);
    int cost(bool print = false);
    std::vector<std::pair<int, int>> choose_most_improved_target();
    std::unordered_map<Signal*, bool> sig_banned_map;

    // copy
    Pshape* copy();
};

class Node {
   public:
    Node();

    ~Node() {
        partial_shapes.clear();
        partial_shapes.shrink_to_fit();
        children.clear();
        children.shrink_to_fit();
        remained_pmos.clear();
        remained_pmos.shrink_to_fit();
    }

    std::string name;
    std::vector<Pshape*> partial_shapes;
    std::vector<Node*> children;
    std::vector<Transistor*> remained_pmos;

    Node* parent;
    Transistor* tr;
    int fill;
    int tr_left_sum;
    int id;
    std::vector<Permutation*> lamb_permutation;
};

class Lambda {
   public:
    Lambda();

    std::string name;
    std::vector<std::vector<int>> config;
    std::vector<std::vector<int>> config_up;
    std::vector<std::vector<int>> config_down;
    std::vector<int> most_left_id;
    int id;
    Transistor* tr;
};

class Permutation {
   public:
    Permutation(){};

    ~Permutation(){};

    std::vector<int> lamb_permutation;
    int size;
    int width;
};

class Group {
   public:
    Group(){};
    ~Group(){};
    std::vector<Transistor*> trs;
    int width;
};

class Group_pair {
   public:
    Group_pair(){};

    ~Group_pair(){};

    std::pair<Group*, Group*> pair;
    int common;
};

#endif