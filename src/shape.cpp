#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include "z3++.h"
#include "cfet.h"

using namespace z3;

int CFET::check_connection(int row, int column, std::vector<bool> bool_vars) {
    std::queue<int> q;
    std::vector<bool> visited;
    visited.assign(row * column, false);
    int num = 0;
    int point_idx;
    for (int i = 0 ; i < bool_vars.size() ; i++) {
        if (bool_vars[i] == true) {
            point_idx = i;
            break;
        }
    }
    q.push(point_idx);
    visited[point_idx] = true;
    while (q.empty() == false) {
        int grid_idx = q.front();
        int row_idx = grid_idx / column;
        int column_idx = grid_idx % column;
        if (row_idx != 0) {
            if (bool_vars[row_idx * column + column_idx - column] == true && visited[row_idx * column + column_idx - column] == false) {
                q.push(row_idx * column + column_idx - column);
                visited[row_idx * column + column_idx - column] = true;
            }
        }
        if (row_idx != row - 1) {
            if (bool_vars[row_idx * column + column_idx + column] == true && visited[row_idx * column + column_idx + column] == false) {
                q.push(row_idx * column + column_idx + column);
                visited[row_idx * column + column_idx + column] = true;
            }
        }
        if (column_idx != column - 1) {
            if (bool_vars[row_idx * column + column_idx + 1] == true && visited[row_idx * column + column_idx + 1] == false) {
                q.push(row_idx * column + column_idx + 1);
                visited[row_idx * column + column_idx + 1] = true;
            }
        }
        if (column_idx != 0) {
            if (bool_vars[row_idx * column + column_idx - 1] == true && visited[row_idx * column + column_idx - 1] == false) {
                q.push(row_idx * column + column_idx - 1);
                visited[row_idx * column + column_idx - 1] = true;
            }
        }
        num++;
        q.pop();
    }
    return num;
}

std::vector<CFET::Shape*> CFET::finger_slot_configuration(int finger) {
    context c;

    solver s(c);

    std::vector<expr> bool_vars;
    std::vector<expr> connection_vars;
    std::vector<std::vector<bool>> solutions;
    std::vector<std::vector<std::vector<bool>>> es;
    std::vector<std::vector<int>> num_shape_of_eachrow;
    std::vector<Shape*> shapes;

    num_shape_of_eachrow.assign(10, std::vector<int>());
    for (auto& row : num_shape_of_eachrow) {
        row.assign(10, 0); 
    }

    // int row = finger;
    int row = 2; // finger slot has maximum height 2
    int column = finger;
    int num_finger = finger;

    if (finger == 1) {
        std::string key = "e_1_1_1";
        std::vector<std::vector<bool>> _es;
        auto it = CFETShapes.find(key);
        if (it != CFETShapes.end()) {
            es.push_back(CFETShapes[key]->config);
            shapes.push_back(CFETShapes[key]);
            return shapes;
        }
        _es.push_back(std::vector<bool> ());
        _es[0].push_back(true);
        Shape* shape = new Shape();
        shape->num_row = 1;
        shape->num_finger = 1;
        shape->id = 1;
        shape->name = "e_1_1_1";
        shape->config = _es;
        CFETShapes.insert(std::make_pair("e_1_1_1", shape));
        es.push_back(_es);
        shapes.push_back(CFETShapes[key]);
        return shapes;
    }

    for (unsigned i = 0; i < row; ++i) {
        for (unsigned j = 0; j < column; ++j) {
            std::stringstream x_name;
            std::stringstream connection_name;
            x_name << "x_" << i << '_' << j;
            connection_name << "c_" << i << '_' << j;
            bool_vars.push_back(c.bool_const(x_name.str().c_str()));
            connection_vars.push_back(c.int_const(connection_name.str().c_str()));
        }
    }

    for (int i = 0; i < connection_vars.size(); ++i) {
        s.add(connection_vars[i] >= 0 && connection_vars[i] <= row * column - 1);
    }

    s.add(connection_vars[0] == 0);


    expr sum = to_expr(c, Z3_mk_int(c, 0, c.int_sort())); // 初始化为 0
    for (int i = 0; i < bool_vars.size(); ++i) {
        sum = sum + ite(bool_vars[i], c.int_val(1), c.int_val(0)); // bool 为 true 时加 1，否则加 0
    }

    for (unsigned i = 0; i < row; ++i) {
        for (unsigned j = 0; j < column; ++j) {
            std::vector<expr> adjacent_vars;
            int idx = i * column + j;
            if (i != 0) {
                adjacent_vars.push_back(bool_vars[i * column + j - column]);
            }
            if (i != row - 1) {
                adjacent_vars.push_back(bool_vars[i * column + j + column]);
            }
            if (j != column - 1) {
                adjacent_vars.push_back(bool_vars[i * column + j + 1]);
            }
            if (j != 0) {
                adjacent_vars.push_back(bool_vars[i * column + j - 1]);
            }

            expr disjunction = adjacent_vars[0];
            for (int i = 1; i < adjacent_vars.size(); ++i) {
                disjunction = disjunction || adjacent_vars[i];
            }
            s.add(implies(bool_vars[idx], disjunction));
        }
    }

    s.add(sum == num_finger);

    expr first_column = bool_vars[0];
    if (row > 1) {
        for (unsigned i = 1; i < row; ++i) {
            first_column = first_column || bool_vars[i * column];
        }
    }
    s.add(first_column);

    expr first_row = bool_vars[0];
    if (column > 1) {
        for (unsigned i = 1; i < column; ++i) {
            first_row = first_row || bool_vars[i];
        }
    }
    s.add(first_row);

    while (s.check() == sat) {
        std::vector<bool> solution;
        model m = s.get_model();
        for (unsigned i = 0; i < row; ++i) {
            for (unsigned j = 0; j < column; ++j) {
                int idx = i * column + j;
                solution.push_back(m.eval(bool_vars[idx]).bool_value() == Z3_L_TRUE);
            }
        }

        int connect_num = check_connection(row, column, solution);

        if (connect_num == num_finger) {
            solutions.push_back(solution);
            for (unsigned i = 0; i < row; ++i) {
                for (unsigned j = 0; j < column; ++j) {
                    int idx = i * column + j;
                    // std::cout << (m.eval(bool_vars[idx]).bool_value() == Z3_L_TRUE ? "1 " : "0 ") << "\t";
                }
                // std::cout << std::endl;
            }
            // std::cout << std::endl;
        }

        expr_vector exclude_current_model(c);
        for (int i = 0; i < bool_vars.size(); ++i) {
            if (solution[i]) {
                exclude_current_model.push_back(!bool_vars[i]);
            } else {
                exclude_current_model.push_back(bool_vars[i]);
            }
        }
        s.add(mk_or(exclude_current_model));
    }
    
    for (int i = 0 ; i < solutions.size() ; i++) {
        std::vector<std::vector<bool>> e;
        e.assign(row, std::vector<bool>());
        for (int j = 0 ; j < row ; j++) {
            for (int k = 0 ; k < column ; k++) {
                e[j].push_back(solutions[i][j * column + k]);
            }
        }
        es.push_back(e);
    }

    int num_row;
    for (int i = 0 ; i < es.size() ; i++) {
        int _finger = 0;
        bool done = false;
        for (int j = 0 ; j < row ; j++) {
            for (int k = 0 ; k < column ; k++) {
                if (es[i][j][k] == true) {
                    _finger++;
                    if (_finger == num_finger) {
                        num_row = j + 1;
                        num_shape_of_eachrow[num_row][num_finger]++;
                        std::stringstream e_name;
                        e_name << "e_" << num_row << '_' << num_finger << '_' << num_shape_of_eachrow[num_row][num_finger];
                        std::string key = e_name.str().c_str();
                        // auto it = CFETShapes.find(key);
                        // if (it == CFETShapes.end()) {
                        //     Shape* shape = new Shape();
                        //     shape->num_row = num_row;
                        //     shape->num_finger = num_finger;
                        //     shape->id = num_shape_of_eachrow[num_row][num_finger];
                        //     shape->name = e_name.str().c_str();
                        //     shape->config = es[i];
                        //     CFETShapes.insert(std::make_pair(e_name.str().c_str(), shape));
                        //     done = true;
                        // }
                            Shape* shape = new Shape();
                            shape->num_row = num_row;
                            shape->num_finger = num_finger;
                            shape->id = num_shape_of_eachrow[num_row][num_finger];
                            shape->name = e_name.str().c_str();
                            shape->config = es[i];
                            shapes.push_back(shape);
                    }
                }
                if (done) break;
            }
            if (done) break;
        }
    }

    // std::cout << "finger: " << finger << std::endl;
    // for (const auto& shape : shapes) {
    //     std::cout << shape->name << std::endl;
    //     for (int j = 0 ; j < row ; j++) {
    //         for (int k = 0 ; k < column ; k++) {
    //             (shape->config[j][k]) ? std::cout << shape->config[j][k] : std::cout << "X";
    //         }
    //         std::cout << std::endl;
    //     }
    //     std::cout << std::endl;
    // }
    return shapes;
}

void CFET::folding_shape_generation() {
    for (Transistor* p: pmos) {
        phi.insert(std::make_pair(p, std::unordered_map<Shape*, std::vector<Lambda*>>()));
        phi_merged.insert(std::make_pair(p, std::unordered_map<Shape*, std::vector<Lambda*>>()));
    }
    for (Transistor* p: pmos) {
        Transistor* n = tr_pairs[p];
        int max_num_finger = std::max(p->num_finger, n->num_finger);
        std::vector<Shape*> p_shapes;
        std::vector<Shape*> n_shapes;
        p_shapes = finger_slot_configuration(p->num_finger);
        n_shapes = finger_slot_configuration(n->num_finger);
        p->es = p_shapes;
        n->es = n_shapes;

        for (Shape* shape: p_shapes) {
            std::vector<Lambda*> lambdas;
            char fifthChar = shape->name[4];
            char thirdChar = shape->name[2];
            int row = thirdChar - '0';
            int column = fifthChar - '0';
            context c;
            solver s(c);
            std::vector<expr> bool_vars;
            std::vector<expr> capacity_vars;
            for (unsigned i = 0; i < row; ++i) {
                for (unsigned j = 0; j < column; ++j) {
                    std::stringstream x_name;
                    std::stringstream c_name;
                    x_name << "x_" << i << '_' << j;
                    c_name << "c_" << i << '_' << j;
                    bool_vars.push_back(c.bool_const(x_name.str().c_str()));  
                    capacity_vars.push_back(c.bool_const(c_name.str().c_str()));   
                }
            }
            for (unsigned i = 0; i < row; ++i) {
                for (unsigned j = 0; j < column; ++j) {
                    if (shape->config[i][j] == false) {
                        s.add(capacity_vars[i * column + j] == c.bool_val(false));
                    }
                    else {
                        s.add(capacity_vars[i * column + j] == c.bool_val(true));
                    }
                }
            }
            for (unsigned i = 0; i < row; ++i) {
                for (unsigned j = 1; j < column; ++j) {
                    s.add(implies(capacity_vars[i * column + j - 1] == c.bool_val(true) && capacity_vars[i * column + j] == c.bool_val(true),
                         bool_vars[i * column + j - 1] != bool_vars[i * column + j]));
                }
            }
            for (unsigned i = 1; i < row; ++i) {
                for (unsigned j = 0; j < column; ++j) {
                    s.add(implies(capacity_vars[(i - 1) * column + j] == c.bool_val(true) && capacity_vars[i * column + j] == c.bool_val(true),
                         bool_vars[(i - 1) * column + j] == bool_vars[i * column + j]));
                }
            }
            int index = 0;
            // generate n and p config
            while (s.check() == sat) {
                Lambda* lamb = new Lambda();
                std::vector<bool> solution;
                std::vector<std::vector<int>> config(row, std::vector<int>(column, 0));
                model m = s.get_model();
                for (unsigned i = 0; i < row; ++i) {
                    for (unsigned j = 0; j < column; ++j) {
                        int idx = i * column + j;
                        solution.push_back(m.eval(bool_vars[idx]).bool_value() == Z3_L_TRUE);
                        if (shape->config[i][j] == false) {
                            // std::cout << "X";
                            config[i][j] = 2;
                        }
                        else {
                            if (m.eval(bool_vars[idx]).bool_value() == Z3_L_TRUE) {
                                // std::cout << "1";
                                config[i][j] = 1;
                            }
                            else {
                                // std::cout << "0";
                                config[i][j] = 0;
                            }
                        }
                    }
                    // std::cout << std::endl;
                }

                std::stringstream lambda_name_p;
                lambda_name_p << "lambda_p_" << index;
                std::string key_p = lambda_name_p.str().c_str();
                lamb->config = config;
                lambdas.push_back(lamb);

                // std::cout << p->name << " " << shape->name << " " << key_p << std::endl;
                //     for (int m = 0; m < config.size(); m++) {
                //         for (int n = 0; n < config[m].size(); n++) {
                //             std::cout << config[m][n];
                //         }
                //         std::cout << std::endl;
                //     }

                expr_vector exclude_current_model(c);
                for (int i = 0; i < bool_vars.size(); ++i) {
                    int n_row = i / column;
                    int n_column = i % column;
                    if (shape->config[n_row][n_column]) {
                        if (solution[i]) {
                            exclude_current_model.push_back(!bool_vars[i]);
                        }
                        else {
                            exclude_current_model.push_back(bool_vars[i]);
                        }
                    }
                }
                s.add(mk_or(exclude_current_model));
                index++;
            }
            phi[p].insert(std::make_pair(shape, lambdas));
        }
        for (Shape* shape: n_shapes) {
            std::vector<Lambda*> lambdas;
            char fifthChar = shape->name[4];
            char thirdChar = shape->name[2];
            int row = thirdChar - '0';
            int column = fifthChar - '0';
            context c;
            solver s(c);
            std::vector<expr> bool_vars;
            std::vector<expr> capacity_vars;
            for (unsigned i = 0; i < row; ++i) {
                for (unsigned j = 0; j < column; ++j) {
                    std::stringstream x_name;
                    std::stringstream c_name;
                    x_name << "x_" << i << '_' << j;
                    c_name << "c_" << i << '_' << j;
                    bool_vars.push_back(c.bool_const(x_name.str().c_str()));  
                    capacity_vars.push_back(c.bool_const(c_name.str().c_str()));   
                }
            }
            for (unsigned i = 0; i < row; ++i) {
                for (unsigned j = 0; j < column; ++j) {
                    if (shape->config[i][j] == false) {
                        s.add(capacity_vars[i * column + j] == c.bool_val(false));
                    }
                    else {
                        s.add(capacity_vars[i * column + j] == c.bool_val(true));
                    }
                }
            }
            for (unsigned i = 0; i < row; ++i) {
                for (unsigned j = 1; j < column; ++j) {
                    s.add(implies(capacity_vars[i * column + j - 1] == c.bool_val(true) && capacity_vars[i * column + j] == c.bool_val(true),
                         bool_vars[i * column + j - 1] != bool_vars[i * column + j]));
                }
            }
            for (unsigned i = 1; i < row; ++i) {
                for (unsigned j = 0; j < column; ++j) {
                    s.add(implies(capacity_vars[(i - 1) * column + j] == c.bool_val(true) && capacity_vars[i * column + j] == c.bool_val(true),
                         bool_vars[(i - 1) * column + j] == bool_vars[i * column + j]));
                }
            }
            int index = 0;
            // generate n and p config
            while (s.check() == sat) {
                Lambda* lamb = new Lambda();
                std::vector<bool> solution;
                std::vector<std::vector<int>> config(row, std::vector<int>(column, 0));
                model m = s.get_model();
                for (unsigned i = 0; i < row; ++i) {
                    for (unsigned j = 0; j < column; ++j) {
                        int idx = i * column + j;
                        solution.push_back(m.eval(bool_vars[idx]).bool_value() == Z3_L_TRUE);
                        if (shape->config[i][j] == false) {
                            // std::cout << "X";
                            config[i][j] = 2;
                        }
                        else {
                            if (m.eval(bool_vars[idx]).bool_value() == Z3_L_TRUE) {
                                // std::cout << "1";
                                config[i][j] = 1;
                            }
                            else {
                                // std::cout << "0";
                                config[i][j] = 0;
                            }
                        }
                    }
                    // std::cout << std::endl;
                }
                std::stringstream lambda_name_n;
                lambda_name_n << "lambda_n_" << index;
                std::string key_n = lambda_name_n.str().c_str();
                lamb->config = config;
                lambdas.push_back(lamb);

                // std::cout << n->name << " " << shape->name << " " << key_n << std::endl;
                //     for (int m = 0; m < config.size(); m++) {
                //         for (int n = 0; n < config[m].size(); n++) {
                //             std::cout << config[m][n];
                //         }
                //         std::cout << std::endl;
                //     }

                expr_vector exclude_current_model(c);
                for (int i = 0; i < bool_vars.size(); ++i) {
                    int n_row = i / column;
                    int n_column = i % column;
                    if (shape->config[n_row][n_column]) {
                        if (solution[i]) {
                            exclude_current_model.push_back(!bool_vars[i]);
                        }
                        else {
                            exclude_current_model.push_back(bool_vars[i]);
                        }
                    }
                }
                s.add(mk_or(exclude_current_model));
                index++;
            }
            phi[n].insert(std::make_pair(shape, lambdas));
        }
            // grnerate lambda n x p

        if (p->num_finger > n->num_finger) {
            for (auto p_shape: p->es) {
                for (auto n_shape: n->es) {
                    std::vector<std::vector<bool>> stack_feasible = stack_feasibility(p_shape, n_shape);
                    phi_merged[p].insert(std::make_pair(n_shape, std::vector<Lambda*>()));
                    for (int x = 0; x < stack_feasible.size(); x++) {
                        for (int y = 0; y < stack_feasible.size(); y++) {
                            if (stack_feasible[x][y]) {
                                for (auto p_lamb: phi[p][p_shape]) {
                                    for (auto n_lamb: phi[n][n_shape]) {
                                        std::vector<std::vector<int>> config_merged;
                                        std::vector<std::vector<int>> n_config_modified;
                                        n_config_modified.assign(p_lamb->config.size(), std::vector<int>(p_lamb->config[0].size()));
                                        for (int _x = 0; _x < n_config_modified.size(); _x++) {
                                            for (int _y = 0; _y < n_config_modified[0].size(); _y++) {
                                                n_config_modified[_x][_y] = 2;
                                            }
                                        }
                                        for (int _x = x; _x < x + n_lamb->config.size(); _x++) {
                                            for (int _y = y; _y < y + n_lamb->config[0].size(); _y++) {
                                                n_config_modified[_x][_y] = n_lamb->config[_x-x][_y-y];
                                            }
                                        }
                                        config_merged.reserve(p_lamb->config.size() * 2);
                                        std::copy(n_config_modified.begin(), n_config_modified.end(), std::back_inserter(config_merged));
                                        std::copy(p_lamb->config.begin(), p_lamb->config.end(), std::back_inserter(config_merged));
                                        
                                        Lambda* lamb_merged = new Lambda();
                                        lamb_merged->config = config_merged;
                                        if (n_shape->name[2] == '1' && p_shape->name[2] == '1') {
                                            lamb_merged->name = "single-row";
                                        }
                                        lamb_merged->config_up = n_config_modified;
                                        lamb_merged->config_down = p_lamb->config;
                                        for (int row = 0; row < lamb_merged->config_up.size(); row++){
                                            for (int left = 0; left < lamb_merged->config_up[row].size(); left++) {
                                                if (lamb_merged->config_up[row][left] != 2 || lamb_merged->config_down[row][left] != 2) {
                                                    lamb_merged->most_left_id.push_back(left);
                                                    break;
                                                }
                                            }
                                        }
                                        phi_merged[p][n_shape].push_back(lamb_merged);
                                        // for (int m = 0; m < config_merged.size(); m++) {
                                        //     for (int n = 0; n < config_merged[m].size(); n++) {
                                        //         std::cout << config_merged[m][n];
                                        //     }
                                        //     std::cout << std::endl;
                                        // }
                                        // std::cout << std::endl;
                                        
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            for (auto p_shape: p->es) {
                for (auto n_shape: n->es) {
                    std::vector<std::vector<bool>> stack_feasible = stack_feasibility(n_shape, p_shape);
                    phi_merged[p].insert(std::make_pair(n_shape, std::vector<Lambda*>()));
                    for (int x = 0; x < stack_feasible.size(); x++) {
                        for (int y = 0; y < stack_feasible[x].size(); y++) {
                            if (stack_feasible[x][y]) {
                                for (auto p_lamb: phi[p][p_shape]) {
                                    for (auto n_lamb: phi[n][n_shape]) {
                                        std::vector<std::vector<int>> config_merged;
                                        std::vector<std::vector<int>> p_config_modified;
                                        p_config_modified.assign(n_lamb->config.size(), std::vector<int>(n_lamb->config[0].size()));
                                        for (int _x = 0; _x < p_config_modified.size(); _x++) {
                                            for (int _y = 0; _y < p_config_modified[0].size(); _y++) {
                                                p_config_modified[_x][_y] = 2;
                                            }
                                        }
                                        for (int _x = x; _x < x + p_lamb->config.size(); _x++) {
                                            for (int _y = y; _y < y + p_lamb->config[0].size(); _y++) {
                                                p_config_modified[_x][_y] = p_lamb->config[_x-x][_y-y];
                                            }
                                        }

                                        config_merged.reserve(n_lamb->config.size() * 2);
                                        std::copy(n_lamb->config.begin(), n_lamb->config.end(), std::back_inserter(config_merged));
                                        std::copy(p_config_modified.begin(), p_config_modified.end(), std::back_inserter(config_merged));
                                        Lambda* lamb_merged = new Lambda();
                                        lamb_merged->config = config_merged;
                                        if (n_shape->name[2] == '1' && p_shape->name[2] == '1') {
                                            lamb_merged->name = "single-row";
                                        }
                                        lamb_merged->config_up = n_lamb->config;
                                        lamb_merged->config_down = p_config_modified;
                                        for (int row = 0; row < lamb_merged->config_up.size(); row++){
                                            for (int left = 0; left < lamb_merged->config_up[row].size(); left++) {
                                                if (lamb_merged->config_up[row][left] != 2 || lamb_merged->config_down[row][left] != 2) {
                                                    lamb_merged->most_left_id.push_back(left);
                                                    break;
                                                }
                                            }
                                        }
                                        phi_merged[p][n_shape].push_back(lamb_merged);
                                        // std::cout << std::endl;
                                        // for (int m = 0; m < config_merged.size(); m++) {
                                        //     for (int n = 0; n < config_merged[m].size(); n++) {
                                        //         std::cout << config_merged[m][n];
                                        //     }
                                        //     std::cout << std::endl;
                                        // }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

std::vector<std::vector<bool>> CFET::stack_feasibility(Shape* big_shape, Shape* small_shape) {
    std::vector<std::vector<bool>> result;
    result.assign(big_shape->config.size(), std::vector<bool>(big_shape->config[0].size()));
    for (int x = 0; x < big_shape->config.size(); x++) {
        for (int y = 0; y < big_shape->config.size(); y++) {
            if (check_overlapped(big_shape, small_shape, x, y)) {
                result[x][y] = true;
            }
            else {
                result[x][y] = false;
            }
        }
    }
    return result;
}

bool CFET::check_overlapped(Shape* big_shape, Shape* small_shape, int x, int y) {
    auto big_config = big_shape->config;
    auto small_config = small_shape->config;
    for (int i = x; i < x + small_shape->config.size(); i++) {
        for (int j = y; j < y + small_shape->config[0].size(); j++) {
            if (small_shape->config[i-x][j-y] == true) {
                // boundary conditions
                if (i >= big_shape->config.size() || j >= big_shape->config[0].size()) {
                    return false;
                }
                if (big_shape->config[i][j] == false) {
                    return false;
                }
            }
        }
    }
    return true;
}