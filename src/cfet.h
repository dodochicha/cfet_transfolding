#ifndef CFET_H_
#define CFET_H_

#include <chrono>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
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
extern std::vector<Signal*> outputs;
extern std::vector<Signal*> inputs;
extern std::vector<Transistor*> trs;
extern std::vector<Transistor*> pmos;
extern std::vector<Transistor*> nmos;
extern std::vector<std::vector<bool>> via_preassignment;
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

void parse_input(std::istream& stream);
void new_tr_pairing();
void folding_shape_generation();
void placement();
void calculate_pgr_blocked(Pshape* pshape);
void print_pshape(Pshape* pshape);
void print_signal_permutation(std::vector<Pshape*> pshape_vec);
void generate_plmt(std::vector<Pshape*> pshape_vec);
void generate_multirow_plmt(std::vector<Pshape*> pshape_vec, std::vector<std::vector<bool>> via_preassignment);
void generateBias(const std::vector<int>& row_bias, std::vector<int>& bias, size_t index, bool& found);
bool check_overlapped(Shape* big_shape, Shape* small_shape, int x, int y);
bool compareGroupPair(const Group_pair* a, const Group_pair* b);
bool comparePshapeHSP(const Pshape* a, const Pshape* b);
bool comparePshapeHSP_descending(const Pshape* a, const Pshape* b);
bool merge_enable(Pshape* old_pshape, Lambda* new_lamb, Transistor* nmos, Transistor* pmos, int row, int col);
bool satisfy_via_rule(std::vector<Pshape*> single_row_vec);
int hsp(Pshape* pshape);
int hcd(Pshape* pshape);
int inter_row_signal_count(std::vector<Pshape*> pshape_vec);
int check_connection(int row, int column, std::vector<bool> bool_vars);
Pshape* merge(Pshape* old_pshape, Lambda* new_lamb, Transistor* nmos, Transistor* pmos, int row, int col);
Pshape* flipped(Pshape* pshape);
Signal* get_right_active(Transistor* tr, int tr_shape_id);
Signal* get_left_active(Transistor* tr, int tr_shape_id);
std::vector<Pshape*> single_row_merging(std::vector<Pshape*> pshape_vec);
std::vector<Shape*> finger_slot_configuration(int finger);
std::vector<Pshape*> group_placement(std::vector<Transistor*> pmos_group);
std::vector<Pshape*> dfs_placement(Node* root_node, int target_area, std::vector<Transistor*> pmos_group);
std::vector<Pshape*> multirow_assignment(std::vector<Pshape*> pshape_vec);
std::vector<std::vector<bool>> stack_feasibility(Shape* big_shape, Shape* small_shape);
std::vector<std::vector<int>> calculate_available_track_case(std::vector<Pshape*> pshape_vec);
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
    int type = 0;  // 0: others, 1: io
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
    std::vector<int> most_right_idx;
    std::vector<int> multirow_pgr_blocked_vdd;
    std::vector<int> multirow_pgr_blocked_vss;
    std::vector<int> available_track_case;
    int multirow_area;
    int height;
    int multirow_macro_area;
    int top_width;
    int hsp;
    int hcd;
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