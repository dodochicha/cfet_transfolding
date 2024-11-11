#ifndef CFET_H_
#define CFET_H_

#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include <map>

class CFET {
    public:
        CFET(const std::map<std::string, float>& attributes);
        int max_cfet_width;
        int diffusion_break_constraint;
        int max_placement_size;
        int max_allowable_cell_height;
        float relaxation_parameter;
        int check_connection(int row, int column, std::vector<bool> bool_vars);

        void parse_input(std::istream& stream);
        void tr_pairing();
        void folding_shape_generation();
        void placement_single_row();
        void placement_single_row_search_tree();
        void placement_multi_row_search_tree();
        void print_layout(std::ostream& stream);
        void custom_pairing();

        void test_ds();
        void test();
        void test_nmos();
    
    private:
        class Shape;
        class Transistor;
        class Signal;
        class Pshape;
        class Node;
        class Lambda;
        enum class MosType;

        std::unordered_map<std::string, Shape*> CFETShapes;
        std::unordered_map<std::string, Signal*> signals;
        std::unordered_map<Transistor*, Transistor*> tr_pairs; // pmos, nmos
        std::unordered_map<std::string, Transistor*> tr_dict;
        // std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::vector<int>>>> phi;
        std::unordered_map<Transistor*, std::vector<std::vector<std::vector<int>>>> single_row_configs;
        std::unordered_map<Transistor*, std::vector<Lambda*>> multi_row_configs;
        std::unordered_map<Transistor*, std::pair<int, int>> layout;
        std::vector<Transistor*> trs;
        std::vector<Transistor*> pmos;
        std::vector<Transistor*> nmos;
        std::unordered_map<Transistor*, std::unordered_map<Shape*, std::vector<Lambda*>>> phi;
        std::unordered_map<Transistor*, std::unordered_map<Shape*, std::vector<Lambda*>>> phi_merged;

        // shape
        std::vector<Shape*> finger_slot_configuration(int finger);

        // util
        std::vector<Signal*> get_right_active_singlerow(Transistor* tr, std::vector<std::vector<int>> config);
        std::vector<Signal*> get_left_active_singlerow(Transistor* tr, std::vector<std::vector<int>> config);
        Signal* get_right_active(Transistor* tr, int tr_shape_id);
        Signal* get_left_active(Transistor* tr, int tr_shape_id);
        bool diffusion_sharing_single_row(std::vector<std::vector<int>> config_left, std::vector<std::vector<int>> config_right, Transistor* tr_left, Transistor* tr_right);
        std::vector<std::vector<bool>> stack_feasibility(Shape* big_shape, Shape* small_shape);
        bool check_overlapped(Shape* big_lambda, Shape* small_lambda, int x, int y);
        bool merge_enable(Pshape* old_pshape, Lambda* new_lamb, Transistor* nmos, Transistor* pmos, int row, int col);
        Pshape* merge(Pshape* old_pshape, Lambda* new_lamb, Transistor* nmos, Transistor* pmos, int row, int col);
        
        // pair
        std::vector<Pshape*> pmos_placement();
        std::vector<Pshape*> nmos_placement();
        Signal* simple_get_right_active(Transistor* tr, int shape_idx);
        Signal* simple_get_left_active(Transistor* tr, int shape_idx);
        

        // layout
        std::vector<int > tr_layout;
        std::vector<std::vector<bool>> ds_layout;
        std::vector<int> tr_shape_idx_layout;
        int width_layout;

        enum class MosType {
            PMOS,
            NMOS
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

        class Transistor {
            public:
                Transistor();

                std::string name;

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

                std::vector<Shape*> es;
                

        };

        class Signal {
            public:
                Signal();

                std::string name;

                

        };

        class Pshape {
            public:
                Pshape();

                ~Pshape() {
                    tr_permutaton.clear();
                    tr_permutaton.shrink_to_fit();
                    tr_shape_id.clear();
                    tr_shape_id.shrink_to_fit();
                    ds_array.clear();
                    ds_array.shrink_to_fit();
                    multirow_tr_shape_up.clear();
                    multirow_tr_shape_up.shrink_to_fit();
                    multirow_tr_shape_down.clear();
                    multirow_tr_shape_down.shrink_to_fit();
                    multirow_tr_permutation_up.clear();
                    multirow_tr_permutation_up.shrink_to_fit();
                    multirow_tr_permutation_down.clear();
                    multirow_tr_permutation_down.shrink_to_fit();
                }

                std::string name;
                std::vector<Transistor*> tr_permutaton;
                std::vector<int> tr_shape_id;
                std::vector<bool> ds_array;
                int width;

                //multi-row
                std::vector<std::vector<int>> multirow_tr_shape_up;
                std::vector<std::vector<int>> multirow_tr_shape_down;
                std::vector<std::vector<Transistor*>> multirow_tr_permutation_up;
                std::vector<std::vector<Transistor*>> multirow_tr_permutation_down;
                std::vector<int> most_right_idx;
                int multirow_area;
                int height;
                int multirow_macro_area;
                int top_width;

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

                std::vector<Pshape*> partial_shapes;
                std::vector<Node*> children;
                std::vector<Transistor*> remained_pmos;
                Node* parent;
                Transistor* tr;
                int fill;

        };

        class Lambda {
            public:
                Lambda();

                std::string name;
                std::vector<std::vector<int>> config;
                std::vector<std::vector<int>> config_up;
                std::vector<std::vector<int>> config_down;
                std::vector<int> most_left_id;

        };
};

#endif  