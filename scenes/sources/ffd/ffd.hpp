#pragma once

#include "scenes/base/base.hpp"


#ifdef SCENE_FFD


struct gui_scene_structure
{
    bool wireframe   = false; // Display the wireframe
    bool shape       = true;  // Display the shape
    bool grid        = true;  // Display the grid
};



struct scene_model : scene_base
{

    // Function applying the deformation - called periodically when the grid is deformed
    void deform_ffd();


    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    void mouse_move(scene_structure& scene, GLFWwindow* window);

    void display_grid(std::map<std::string,GLuint>& shaders, scene_structure& scene);
    void set_gui();

    gui_scene_structure gui_scene;

    vcl::mesh shape;           // The initial shape to be deformed
    vcl::mesh_drawable visual; // The visual representation of the deformed shape

    // Parameters of the grid
    vcl::buffer3D<vcl::vec3> grid;   // The grid where each position is stored as grid(x,y,z)
    vcl::vec3 grid_length;           // The length of the grid corner
    vcl::vec3 grid_center;           // The center of the grid

    vcl::mesh_drawable sphere_grid;                      // Visual representation of the grid positions
    vcl::segment_drawable_immediate_mode segment_drawer; // Visual representation of the grid edges

    int picked_object; // Store the index of the picked object - in this case the index of the position in the grid


    vcl::timer_event timer_update_deform; // Timer used to call the deformation function
    bool is_deformed = false;             // Variable set to true when the grid is deformed



};

#endif


