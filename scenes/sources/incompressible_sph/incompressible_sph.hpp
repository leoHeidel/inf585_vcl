#pragma once

#include "scenes/base/base.hpp"

#ifdef INCOMPRESSIBLE_SPH

// SPH Particle
struct particle_element
{
    vcl::vec3 p; // Position
    vcl::vec3 v; // Speed
    vcl::vec3 dp; // Position correction
    vcl::vec3 q; // New position
    float lambda; // Constraint
    std::vector<size_t> neighbors; // Neighbor indices
    float rho;

    particle_element() : p{0,0,0},v{0,0,0}, dp{0,0,0}, lambda(0.f), rho(0.f) {}
};

// SPH simulation parameters
struct sph_parameters
{
    float h;     // influence distance of a particle
    float rho0;  // rest density
    float m;     // total mass of a particle
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

    void initialize_sph();
    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void display(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    std::vector<particle_element> particles;
    sph_parameters sph_param;

    void set_gui();

    gui_parameters gui_param;
    vcl::mesh_drawable sphere;
    vcl::segments_drawable borders;

    float W(const vcl::vec3 & p);
    vcl::vec3 gradW(const vcl::vec3 & p);

    void apply_force(size_t i, float dt);
    void predict_position(size_t i);
    void find_neighbors();
    void compute_constraints();
    void compute_dP(size_t i);
    void solve_collision(size_t i);
    void add_position_correction();
    void update_velocity(size_t i, float dt);
    void apply_vorticity(size_t i);
    void apply_viscosity(size_t i);
    void update_position(size_t i);

    vcl::timer_event timer;
};


#endif
