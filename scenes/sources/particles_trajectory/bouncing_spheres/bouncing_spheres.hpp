#pragma once

#include "scenes/base/base.hpp"

#ifdef SCENE_PARTICLES_TRAJECTORY_BOUNCING_SPHERES


struct particle_structure
{
    vcl::vec3 p0; // Initial position
    vcl::vec3 v0; // Initial speed
    float t0;     // Initial time of creation

    vcl::curve_dynamic_drawable trajectory; // Posibility to store its recent trajectory
};

struct scene_model : scene_base
{
    // List of all particles
    //  The use of a list allows to add/remove particles in any order
    std::list<particle_structure> particles;

    vcl::mesh_drawable sphere; // Visual representation of a particle

    vcl::mesh_drawable ground; // Visual representation of the ground

    vcl::timer_event timer; // Timer use to compute evolving time and indicates when a new particle should be emitted


    // Initialise data
    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    // Update data at each new frame
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    // Setup GUI parameters
    void set_gui();

};

#endif
