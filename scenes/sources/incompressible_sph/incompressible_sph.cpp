
#include "incompressible_sph.hpp"

#include <random>

#ifdef INCOMPRESSIBLE_SPH
using namespace vcl;

// Counter used to save image on hard drive
int counter_image = 0;

void scene_model::initialize_sph()
{
    // Influence distance of a particle (size of the kernel)
    const float h = 0.1f;


    // Rest density (consider 1000 Kg/m^3)
    const float rho0 = 1000.0f;

    // Stiffness (consider ~2000 - used in tait equation)
    const float stiffness = 2000.0f;

    // Viscosity parameter
    const float nu = 2.0f;

    // Total mass of a particle (consider rho0 h^2)
    const float m = rho0*h*h;

    // Initial particle spacing (relative to h)
    const float c = 0.95f;


    // Fill a square with particles
    const float epsilon = 1e-3f;
    for(float x=h; x<1.0f-h; x=x+c*h)
    {
        for(float y=-1.0f+h; y<0.0f-h; y=y+c*h)
        {
            particle_element particle;
            particle.p = {x+epsilon*rand_interval(),y,0}; // a zero value in z position will lead to a 2D simulation
            particles.push_back(particle);
        }
    }

    sph_param.rho0 = rho0;
    sph_param.m    = m;
}



void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    const float dt = timer.update();
    set_gui();

    // Force constant time step
    float h = dt<=1e-6f? 0.0f : timer.scale*0.0003f;

    for(int i=0; i < particles.size(); ++i){
      apply_force(i);
      predict_position(i);
    }
    find_neighbors();
    while(int iter=0; iter<solverIterations; ++iter){
      compute_constraints();
      for(int i=0; i < particles.size(); ++i){
        compute_dP(i);
        solve_collision(i);
      }
      add_position_correction();
    }
    for(int i=0; i < particles.size(); ++i){
      update_velocity(i);
      apply_vorticity(i);
      apply_viscosity(i);
      update_position(i);
    }


    display(shaders, scene, gui);
}

void scene_model::apply_force(size_t i){

}

void scene_model::predict_position(size_t i){

}

void scene_model::find_neighbors(){

}

void scene_model::compute_constraints(){

}

void scene_model::compute_dP(size_t i){

}

void scene_model::solve_collision(size_t i){

}

void scene_model::add_position_correction(){

}

void scene_model::update_velocity(size_t i){

}

void scene_model::apply_vorticity(size_t i){

}

void scene_model::apply_viscosity(size_t i){

}

void scene_model::update_position(size_t i){

}

void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& , gui_structure& gui)
{
    gui.show_frame_camera = false;

    sphere = mesh_drawable( mesh_primitive_sphere(1.0f));
    sphere.shader = shaders["mesh"];
    sphere.uniform.color = {0,0.5,1};

    std::vector<vec3> borders_segments = {{-1,-1,-1},{1,-1,-1}, {1,-1,-1},{1,1,-1}, {1,1,-1},{-1,1,-1}, {-1,1,-1},{-1,-1,-1},
                                          {-1,-1,1} ,{1,-1,1},  {1,-1,1}, {1,1,1},  {1,1,1}, {-1,1,1},  {-1,1,1}, {-1,-1,1},
                                          {-1,-1,-1},{-1,-1,1}, {1,-1,-1},{1,-1,1}, {1,1,-1},{1,1,1},   {-1,1,-1},{-1,1,1}};
    borders = segments_gpu(borders_segments);
    borders.uniform.color = {0,0,0};
    borders.shader = shaders["curve"];


    initialize_sph();
    sphere.uniform.transform.scaling = sph_param.h/5.0f;

    gui_param.display_field = true;
    gui_param.display_particles = true;
    gui_param.save_field = false;

}


void scene_model::display(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    draw(borders, scene.camera);

    // Display particles
    if(gui_param.display_particles)
    {
        const size_t N = particles.size();
        for(size_t k=0; k<N; ++k) {
            sphere.uniform.transform.translation = particles[k].p;
            draw(sphere, scene.camera);
        }
    }
}



void scene_model::set_gui()
{
    // Can set the speed of the animation
    float scale_min = 0.05f;
    float scale_max = 2.0f;
    ImGui::SliderScalar("Time scale", ImGuiDataType_Float, &timer.scale, &scale_min, &scale_max, "%.2f s");

    ImGui::Checkbox("Display field", &gui_param.display_field);
    ImGui::Checkbox("Display particles", &gui_param.display_particles);
    ImGui::Checkbox("Save field on disk", &gui_param.save_field);

    // Start and stop animation
    if (ImGui::Button("Stop"))
        timer.stop();
    if (ImGui::Button("Start"))
        timer.start();
}




#endif
