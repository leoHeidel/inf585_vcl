#pragma once


#include "scenes/base/base.hpp"
#ifdef SCENE_STABLE_FLUID

#include "vcl/containers/buffer/buffer.hpp"


using velocity_grid = vcl::buffer2D<vcl::vec2>;
using scalar_grid = vcl::buffer2D<float>;

enum density_type_structure {density_color, density_texture} ;

struct gui_scene_structure
{
    bool display_grid = true;
    bool display_velocity = true;
    bool display_density = true;
    density_type_structure density_type = density_color;
};



struct scene_model : scene_base
{

    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    void mouse_move(scene_structure& scene, GLFWwindow* window);
    void display_velocity(GLuint shader, scene_structure& scene);
    void initialize_fields(density_type_structure density_type);

    void set_gui();
    void fluid_evolve(float dt);


    vcl::screen_motion_structure screen_motion;

    velocity_grid velocity;
    velocity_grid velocity_prev;



    vcl::buffer2D<vcl::vec3> density, density_save;



    vcl::segments_drawable visual_grid;
    void setup_visual_grid(int Nx, int Ny, GLuint shader);



    GLuint texture_id;
    vcl::mesh_drawable quad;

    float diffuse_coefficient_velocity = 0.01f;
    float diffuse_coefficient_density = 0.0f;

    scalar_grid divergence;
    scalar_grid gradient_field;


    vcl::segment_drawable_immediate_mode segment_drawer;


    vcl::timer_basic timer;
    gui_scene_structure gui_scene;

};

#endif


