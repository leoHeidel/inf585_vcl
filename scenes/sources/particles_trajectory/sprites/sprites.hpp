#pragma once

#include "scenes/base/base.hpp"

#ifdef SCENE_PARTICLES_TRAJECTORY_SPRITES

struct particle_structure
{

    vcl::vec3 p0;
    float t0;

    // Add parameters you need to store for the particles

    vcl::curve_dynamic_drawable trajectory;
};

struct gui_parameters
{
    bool display_decoration = true;
};

struct scene_model : scene_base
{
    std::list<particle_structure> particles_sprite;
    std::list<particle_structure> particles_bubble;

    vcl::mesh_drawable cooking_pot;
    vcl::mesh_drawable spoon;

    vcl::mesh_drawable sprite;
    vcl::mesh_drawable bubble;

    vcl::timer_event timer_bubble;
    vcl::timer_event timer_sprite;


    vcl::mesh_drawable sphere;
    gui_parameters gui_scene;





    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    void load_decoration_meshes(GLuint shader);
    void set_gui();

};

#endif
