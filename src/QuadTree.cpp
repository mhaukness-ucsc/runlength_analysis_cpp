
#include "QuadTree.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>

using std::ofstream;
using std::make_shared;
using std::runtime_error;
using std::cout;
using std::cerr;
using std::to_string;
using std::experimental::filesystem::create_directories;


QuadTree::QuadTree(){
    this->boundary = BoundingBox(QuadCoordinate(0,0), 0);
}


QuadTree::QuadTree(BoundingBox boundary){
    this->boundary = boundary;
}


void QuadTree::redistribute_points(){
    uint8_t index;

    while (not this->points.empty()){
        index = this->find_quadrant(this->points.back());
        this->subtrees[index]->insert(this->points.back());
        this->points.pop_back();
    }
}

void QuadTree::update_loss_from_range(BoundingBox& bounds, QuadLoss& loss_calculator){
    return;
}

shared_ptr<QuadTree> QuadTree::generate_child(BoundingBox bounds){
    return make_shared<QuadTree>(bounds);
}


void QuadTree::subdivide(){
    // Find new half size
    float new_half_size = this->boundary.half_size/2;

    // Find boundaries
    float x_left = this->boundary.center.x - new_half_size;
    float x_right = this->boundary.center.x + new_half_size;
    float y_bottom = this->boundary.center.y - new_half_size;
    float y_top = this->boundary.center.y + new_half_size;

    // Top left
    QuadCoordinate top_left_center = QuadCoordinate(x_left, y_top);
    BoundingBox top_left_bounds = BoundingBox(top_left_center, new_half_size);
    this->subtrees[QuadTree::TOP_LEFT] = generate_child(top_left_bounds);

    // Top right
    QuadCoordinate top_right_center = QuadCoordinate(x_right, y_top);
    BoundingBox top_right_bounds = BoundingBox(top_right_center, new_half_size);
    this->subtrees[QuadTree::TOP_RIGHT] = generate_child(top_right_bounds);

    // Bottom left
    QuadCoordinate bottom_left_center = QuadCoordinate(x_left, y_bottom);
    BoundingBox bottom_left_bounds = BoundingBox(bottom_left_center, new_half_size);
    this->subtrees[QuadTree::BOTTOM_LEFT] = generate_child(bottom_left_bounds);

    // Bottom right
    QuadCoordinate bottom_right_center = QuadCoordinate(x_right, y_bottom);
    BoundingBox bottom_right_bounds = BoundingBox(bottom_right_center, new_half_size);
    this->subtrees[QuadTree::BOTTOM_RIGHT] = generate_child(bottom_right_bounds);

    this->redistribute_points();
}


uint8_t QuadTree::find_quadrant(QuadCoordinate c){
    uint8_t quadrant = QuadTree::NOT_FOUND;

    // Top
    if (c.y >= this->boundary.center.y){
        // Inside top bounds
        if (c.y < this->boundary.center.y + this->boundary.half_size) {
            // Right
            if (c.x >= this->boundary.center.x){
                // Inside right bounds
                if (c.x < this->boundary.center.x + this->boundary.half_size) {
                    // Top right!
                    quadrant = QuadTree::TOP_RIGHT;
                }
            }

            // Left
            else {
                // Inside left bounds
                if (c.x >= this->boundary.center.x - this->boundary.half_size) {
                    // Top left!
                    quadrant = QuadTree::TOP_LEFT;
                }
            }
        }
    }
    // Bottom
    else {
        // Inside bottom bounds
        if (c.y >= this->boundary.center.y - this->boundary.half_size) {
            // Right
            if (c.x >= this->boundary.center.x){
                // Inside right bounds
                if (c.x < this->boundary.center.x + this->boundary.half_size) {
                    // Bottom right!
                    quadrant = QuadTree::BOTTOM_RIGHT;
                }
            }

            // Left
            else {
                // Inside left bounds
                if (c.x >= this->boundary.center.x - this->boundary.half_size) {
                    // Bottom left!
                    quadrant = QuadTree::BOTTOM_LEFT;
                }
            }
        }
    }

    return quadrant;
}


bool QuadTree::insert(QuadCoordinate c){
    bool success = false;

    // Check whether the point is within one of the quadrants of this tree, if so return the quadrant
    uint8_t index = this->find_quadrant(c);

    if (index != QuadTree::NOT_FOUND) {
        // If this quadtree is below capacity and it has not formed any children, just append the point
        if (this->points.size() < QuadTree::capacity and this->subtrees[0] == nullptr) {
            this->points.push_back(c);
            success = true;
        }

        // If capacity has been exceeded, and children need to be spawned, spawn them and pass the point onward
        else if (this->subtrees[0] == nullptr){
            this->subdivide();
            success = this->subtrees[index]->insert(c);
        }

        // If this tree already has children and exceeded capacity, pass the point onward
        else {
            success = this->subtrees[index]->insert(c);
        }
    }

    return success;
}


void QuadTree::query_range(vector<QuadCoordinate>& results, BoundingBox& bounds){
    //TODO: rewrite for non-square rectangles

    // Find boundaries, for constructing corners
    float x_left = this->boundary.center.x - this->boundary.half_size;
    float x_right = this->boundary.center.x + this->boundary.half_size;
    float y_bottom = this->boundary.center.y - this->boundary.half_size;
    float y_top = this->boundary.center.y + this->boundary.half_size;

    // Check if there is no intersection, terminate
    // TODO: change this lazy implementation to be a decision tree
    if (find_quadrant(QuadCoordinate(x_left,y_bottom)) == QuadTree::NOT_FOUND and
        find_quadrant(QuadCoordinate(x_left,y_top)) == QuadTree::NOT_FOUND and
        find_quadrant(QuadCoordinate(x_right,y_bottom)) == QuadTree::NOT_FOUND and
        find_quadrant(QuadCoordinate(x_right,y_top)) == QuadTree::NOT_FOUND){
        return;
    }

    // If there is an intersection and some of the points are in range, add them
    QuadTree query_quad(bounds);
    for (auto& point: this->points){
        if (query_quad.find_quadrant(point) != QuadTree::NOT_FOUND){
            results.push_back(point);
        }
    }

    if (this->subtrees[0] == nullptr){
        return;
    }
    else{
        for (auto& subtree: this->subtrees){
            subtree->query_range(results, bounds);
        }
    }
}


void QuadTree::append_dot_string(string& edges, string& labels, uint64_t& n){
    string name;
    string label;
    uint64_t n_parent = n;

    size_t i = 0;
    for (auto& subtree: this->subtrees){
        if (not subtree){
            continue;
        }

        n += 1;
        label = to_string(subtree->points.size());
        name = to_string(n);

        edges += "\n\t" + to_string(n_parent) + "->" + name + ";";

        if (not subtree->points.empty()){
            labels += "\t" + name + "[label=\"" + label + "\"];\n";
        }
        else{
            labels += "\t" + name + "[label=\"\"];\n";
        }

        subtree->append_dot_string(edges, labels, n);

        i++;
    }
}


void QuadTree::write_as_dot(path output_dir, string suffix, bool plot){
    output_dir = absolute(output_dir);
    path output_path = output_dir / ("quad_tree_" + suffix + ".dot");
    create_directories(output_dir);
    ofstream file = ofstream(output_path);

    string dot_string = "digraph G {\n";
    string edges;
    string labels;

    uint64_t n = 0;

    this->append_dot_string(edges, labels, n);

    dot_string += labels;
    dot_string += edges;
    dot_string += "\n}\n";

    file << dot_string;

    if (plot){
        path plot_path = output_dir / ("quad_tree_" + suffix + ".pdf");
        string command = "dot -Tps " + output_path.string() + " -o " + plot_path.string();
        cerr << "plotting with graphviz: " + command + "\n";

        int exit_code = system(command.c_str());

        if (exit_code != 0){
            throw runtime_error("ERROR: command failed to run: " + command);
        }
    }
}


void QuadTree::append_bounds_string(string& bounds){
    size_t i = 0;

    if (not this->subtrees[0]) {
        bounds += to_string(this->boundary.center.x) + "," + to_string(this->boundary.center.y) + "\t" +
                  to_string(this->boundary.half_size) + "\n";
    }

    for (auto& subtree: this->subtrees){
        if (not subtree){
            continue;
        }

        subtree->append_bounds_string(bounds);

        i++;
    }
}


void QuadTree::write_bounds(path output_dir, string suffix){
    output_dir = absolute(output_dir);
    ofstream file = ofstream(output_dir / ("quad_tree_bounds_" + suffix + ".txt"));
    create_directories(output_dir);
    string bounds;

    this->append_bounds_string(bounds);

    file << bounds;
}
