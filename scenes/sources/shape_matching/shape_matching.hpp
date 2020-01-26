#pragma once

#include "scenes/base/base.hpp"
#ifdef SCENE_SHAPE_MATCHING

#include "shape_matching_object.hpp"
#include <map>





// Define the three possible surfaces: Cube, Cylinder, Torus
enum surface_type_enum {surface_cube, surface_cylinder, surface_torus};

struct gui_scene_structure{
    bool sphere;
    bool wireframe;
    float speed_accumulator;
    float speed_accumulator_max;
    bool speed_loading;
    float radius_bounding_sphere;
    surface_type_enum surface_type;
};


struct scene_model : scene_base
{

    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void initialize_shapes();

    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    // Can insert new shape using space key
    void keyboard_input(scene_structure& scene, GLFWwindow* window, int key, int scancode, int action, int mods);
    void throw_new_shape(const scene_structure& scene);

    void set_gui();


    // Helper functions
    void numerical_integration(float dt);
    void position_as_speed(float dt);
    void display_shapes_surface(std::map<std::string,GLuint>& shaders, scene_structure& scene);
    void display_bounding_spheres(std::map<std::string,GLuint>& shaders, scene_structure& scene);

    // The set of shapes
    std::vector<shape_matching_object> shapes;
    float object_length; // Caracteristic length of each shape

    // Storage for all the possible basic shapes (cube, cylinder, torus)
    std::map<surface_type_enum, vcl::mesh> mesh_basic_model;

    // Visual model of the bounding spheres
    vcl::mesh_drawable sphere_visual;

    // Timer for the simulation
    vcl::timer_basic timer;

    // Visual model for the border
    vcl::segments_drawable border_visual;
    float border_length;

    // Specific GUI for this scene
    gui_scene_structure gui_scene;
};

#endif


