

#include "incompressible_sph.hpp"

#include <random>
#include <unordered_map>
#include <cmath>
#include <algorithm>

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

    // Total mass of a particle (consider rho0 h^3)
    const float m = rho0*h*h*h;

    // Initial particle spacing (relative to h)
    const float c = 1.2f;

    // Fill a square with particles
    const float epsilon = 1e-3f;
    // float dist = 0;
    const int N_PARTICLES = 3500;

    std::default_random_engine generator;
    std::normal_distribution<float> normal(0,1);
    for (size_t i = 0; i < N_PARTICLES; i++)
    {
       vec3 v = {normal(generator), normal(generator), normal(generator)};
       particle_element particle;
       particle.p = 0.03*v;
       particles.push_back(particle);
    }

    sph_param.h    = h;
    sph_param.rho0 = rho0;
    sph_param.m    = m;
    sph_param.eps  = epsilon;

    oclHelper.init_context();
    // oclHelper.test_context();
}

void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    count++;
    if (count > 50) {
        const float dt = 0.02f;
        // const float dt = timer.update();
        set_gui();

        // Force constant time step
        size_t solverIterations = 3;

        std::vector<vec3> v;
        for (auto &part : particles)
        {
            v.push_back(part.v);
        }

        std::vector<vec3> positions;
        for (auto &part : particles)
        {
            positions.push_back(part.p);
        }
        oclHelper.befor_solver(positions, v);

        oclHelper.make_neighboors();
        size_t k=0;

        while(k<solverIterations){
        oclHelper.solver_step();
        ++k;
        }

        oclHelper.update_speed();

        std::vector<vcl::vec3> p_gpu = oclHelper.get_p();
        for (size_t i = 0; i < particles.size(); i++)
        {
            particles[i].p = p_gpu[i];
        }

        std::vector<vcl::vec3> v_gpu = oclHelper.get_v();
        for (size_t i = 0; i < particles.size(); i++)
        {
            particles[i].v = v_gpu[i];
        }
    }
    display(shaders, scene, gui);
}

// SPH Smooth Kernel
// homogeneous to h^-3
float scene_model::W(const vcl::vec3 & p){
    float d = norm(p);
    if(d<=sph_param.h){
        float C = 315./(64.*M_PI*powf(sph_param.h,3.));
        float a = d/sph_param.h;
        float b = 1-a*a;
        return float(C*powf(b,3.));
    }
    return 0.f;
}

//homogeneous to h^-4
vcl::vec3 scene_model::gradW(const vcl::vec3 & p){
    float d = norm(p);
    if(d<=sph_param.h){
        float C = -6.f*315.f/(64.f*M_PI*powf(sph_param.h,5.f));
        float a = d/sph_param.h;
        float b = 1-a*a;
        return C*powf(b,2.f)*p;
    }else{
        return vec3(0.f,0.f,0.f);
    }
}

//homogeneous to h^-4
vcl::vec3 scene_model::gradW_spkiky(const vcl::vec3 & p){
    float d = norm(p);
    if(d<=sph_param.h){
        float B = 45 / M_PI * powf(sph_param.h, -6.f);
        float a = sph_param.h - d;
        return -B* a*a *p/d;
    }else{
        return vec3(0.f,0.f,0.f);
    }
}

int hash_function(float x, float y, float z) {
    //Hash function for three integers, used for the neightboor search
    int int_x = static_cast <int> (std::floor(x));
    int int_y = static_cast <int> (std::floor(y));
    int int_z = static_cast <int> (std::floor(z));
    return (int_x*11969 + int_y)*80737+int_z;
}

void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& , gui_structure& gui)
{
    gui.show_frame_camera = false;

    // sphere = mesh_drawable( mesh_primitive_sphere(1.0f));
    // sphere.shader = shaders["mesh"];
    // sphere.uniform.color = {0,0.5,1};

    disc = mesh_drawable( mesh_primitive_disc(1.0f));
    disc.shader = shaders["fluid"];
    disc.uniform.color = {0,0.5,1};

    std::vector<vec3> borders_segments = {{-1,-1,-1},{1,-1,-1}, {1,-1,-1},{1,1,-1}, {1,1,-1},{-1,1,-1}, {-1,1,-1},{-1,-1,-1},
                                          {-1,-1,1} ,{1,-1,1},  {1,-1,1}, {1,1,1},  {1,1,1}, {-1,1,1},  {-1,1,1}, {-1,-1,1},
                                          {-1,-1,-1},{-1,-1,1}, {1,-1,-1},{1,-1,1}, {1,1,-1},{1,1,1},   {-1,1,-1},{-1,1,1}};
    borders = segments_gpu(borders_segments);
    borders.uniform.color = {0,0,0};
    borders.shader = shaders["curve"];

    initialize_sph();
    //sphere.uniform.transform.scaling = sph_param.h / 2;
    disc.uniform.transform.scaling = sph_param.h / 2;

    gui_param.display_field = true;
    gui_param.display_particles = true;
    gui_param.save_field = false;
}


void scene_model::display(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    draw(borders, scene.camera);

    const size_t N = particles.size();
    for(size_t k=0; k<N; ++k) {
        //sphere.uniform.transform.translation = particles[k].p;
        disc.uniform.transform.translation = particles[k].p;
        disc.uniform.transform.rotation = scene.camera.orientation;
        draw(disc, scene.camera);
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
