#pragma once

#include <chrono> 

#include "scenes/base/base.hpp"
#include "opencl_helper.hpp"

#ifdef INCOMPRESSIBLE_SPH

// SPH Particle
struct particle_element
{
    vcl::vec3 p; // Position
    vcl::vec3 v; // Speed
    particle_element() : p{0,0,0},v{0,0,0} {}
};



// Image used to display the water appearance
struct field_display
{
    vcl::image_raw im;       // Image storage on CPU
    GLuint texture_id;       // Texture stored on GPU
    vcl::mesh_drawable quad; // Mesh used to display the texture
};

// User parameters available in the GUI
struct gui_parameters
{
    bool display_field;
    bool display_particles;
    bool save_field;
};


struct scene_model : scene_base
{
    int count = 0;

    float alpha_time = 0.97;
    float pre_solver_time;
    float neighboors_time;
    float solver_time;
    float post_solver_time;
    float render_time;
    float total_time;

    OCLHelper oclHelper;
    void initialize_sph();
    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void display(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    std::vector<particle_element> particles;
    sph_parameters sph_param;

    void set_gui();

    gui_parameters gui_param;
    vcl::mesh_drawable sphere;
    vcl::mesh_drawable billboard;
    vcl::segments_drawable borders;
    vcl::segments_drawable spheres;

    vcl::timer_event timer;
};


#endif
