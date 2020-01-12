#pragma once


#include "scenes/base/base.hpp"
#ifdef SCENE_LAPLACIAN_EDITING

// Include Eigen
#define EIGEN_NO_DEBUG
#define EIGEN_INITIALIZE_MATRICES_BY_ZERO
#include "Eigen/Sparse"


#include <map>
#include <set>


struct gui_scene_structure
{
    bool wireframe = true;


};

// Handling constraints using picking
struct picking_structure
{
    std::set<int> constraints_temporary;       // Indices of position currently in the selection cursor (before releasing the mouse)
    vcl::vec2 cursor_previous;  // Store previous cursor position to compute displacement

    vcl::vec2 selection_p0;  // Screen position of the first corner of the selection
    vcl::vec2 selection_p1;  // Screen position of the second corner of the selection
    vcl::segment_drawable_immediate_mode segment_drawer; // used to display the temporary selection cursor
};

// 2 types of constraints:
//  - Fixed points (associated to weight_target)
//  - Target points that can be displaced (associated to weight_target)
struct constraint_structure {

    // Store constraints as (index in mesh, 3D target position)
    std::map<int, vcl::vec3> fixed;
    std::map<int, vcl::vec3> target;

    // Associated weights for the least square solution
    float weight_fixed  = 2.0f; // fixed points are almost hard constraints
    float weight_target = 0.1f; // smaller weights for target points
};


struct scene_model : scene_base
{
    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    void mouse_click(scene_structure& scene, GLFWwindow* window, int button, int action, int mods);
    void mouse_move(scene_structure& scene, GLFWwindow* window);
    void keyboard_input(scene_structure& scene, GLFWwindow* window, int key, int scancode, int action, int mods);

    void set_gui();

    void initialize_shape();
    void build_matrix();
    void update_deformation();

    void diplay_selector_rectangle(scene_structure const& scene, std::map<std::string,GLuint> const& shaders);
    void display_constraints(scene_structure const& scene);


    vcl::mesh shape;                      // Deformable shape
    bool need_update = false;             // Indicates if the shape should be deformed (after change of the constraints)

    bool constraints_selection_mode = false; // Indicate the current mode: Displacement / Selection
    constraint_structure constraints;     // Store constraints data: position and weights
    picking_structure picking;            // Storage for picking and constrained vertices

    vcl::mesh_drawable visual;            // Visual representation of the shape
    GLuint shader_mesh;
    vcl::mesh_drawable sphere;            // Visual representation of the constraints and picked vertices
    gui_scene_structure gui_scene;

    vcl::timer_event timer;               // Timer associated to shape update


};

#endif


