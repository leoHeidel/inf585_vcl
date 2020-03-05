

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
    const float rho0 = 10.0f;

    // Viscosity parameter
    const float nu = 2.0f;

    // Total mass of a particle (consider rho0 h^2)
    const float m = rho0*h*h;

    // Initial particle spacing (relative to h)
    const float c = 0.9f;


    // Fill a square with particles
    const float epsilon = 1e-3f;
    float dist = 2.5*h*c;
    for(float x=-dist; x<=dist+h/10; x+=c*h)
    {
        for(float z=-dist; z<=dist+h/10; z+=c*h)
        {
            for (float y=2*h-1; y<2*h+2*dist+h/10-1; y+=c*h)
            {
                particle_element particle;
                particle.p = {x+epsilon*rand_interval(),y,z}; // a zero value in z position will lead to a 2D simulation
                particles.push_back(particle);
            }
        }
    }

    sph_param.h    = h;
    sph_param.rho0 = rho0;
    sph_param.m    = m;
}



void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    const float dt = timer.update();
    set_gui();

    // Force constant time step
    float h = dt<=1e-6f? 0.0f : timer.scale*0.0003f;
    size_t solverIterations = 6;

    for(size_t i=0; i < particles.size(); ++i){
      particles[i].v += dt * vcl::vec3(0.f, -0.2f, 0.f);
      particles[i].q = particles[i].p + dt * particles[i].v;
    }
    find_neighbors();
    size_t k=0;
    while(k<solverIterations){
      compute_constraints();
      for(size_t i=0; i < particles.size(); ++i){
        compute_dP(i);
        solve_collision(i, dt);
      }
      add_position_correction();
      ++k;
    }
    for(size_t i=0; i < particles.size(); ++i){
      update_velocity(i, dt);
      apply_vorticity(i);
      apply_viscosity(i);
      update_position(i);
    }


    display(shaders, scene, gui);
}

// SPH Smooth Kernel
float scene_model::W(const vcl::vec3 & p){
    if(norm(p)<=sph_param.h){
        float C = 315./(64.*M_PI*pow(double(sph_param.h),3.));
        float a = norm(p)/sph_param.h;
        float b = 1-a*a;
        return float(C*powf(b,3.));
    }
    return 0.f;
}

vcl::vec3 scene_model::gradW(const vcl::vec3 & p){
    if(norm(p)<=sph_param.h){
        float C = 315.f/(64.f*3.14159265f*powf(sph_param.h,3.f));
        float a = norm(p)/sph_param.h;
        float b = 1-a*a;
        return -6.f/powf(sph_param.h,2.f)*C*powf(b,2.f)*p;
    }else{
        return vec3(0.f,0.f,0.f);
    }
}

int hash_function(int x, int y, int z) {
    //Hash function for three integers, used for the neightboor search
    int int_x = static_cast <int> (std::floor(x));
    int int_y = static_cast <int> (std::floor(y));
    int int_z = static_cast <int> (std::floor(z));
    return (int_x*11969 + int_y)*80737+int_z;
}

void scene_model::find_neighbors(){
    //Return all the neigbors within distance h. Might return neightbors up to distance 2*h. Use a hashtable to be in complexity close to linear. 
    std::unordered_map<int, std::vector<size_t>> hashmap(particles.size());
    for (size_t i = 0; i < particles.size(); i++)
    {
        int hash = hash_function(particles[i].p.x/sph_param.h, particles[i].p.y/sph_param.h, particles[i].p.z/sph_param.h);
        hashmap[hash].push_back(i);
        particles[i].neighbors.clear();
    }

    for (size_t i = 0; i < particles.size(); i++)
    {
        float x = particles[i].p.x/sph_param.h;
        float y = particles[i].p.y/sph_param.h;
        float z = particles[i].p.z/sph_param.h;

        for (int dx = -1; dx < 2; dx++)
        {
            for (int dy = -1; dy < 2; dy++)
            {
                for (int dz = -1; dz < 2; dz++)
                {
                    int hash = hash_function(x+dx,y+dy,z+dz);
                    auto iter = hashmap.find(hash);
                    if (iter != hashmap.end())
                    {
                        for (auto &&j : iter->second)
                        {
                            float d = norm(particles[i].p - particles[j].p);
                            if (j != i && d < sph_param.h) particles[i].neighbors.push_back(j);
                        }
                    }
                }
            }
        }
    }
}

void scene_model::compute_constraints(){
  for(particle_element & particle : particles){
    particle.rho = 0.f;
    vcl::vec3 ci = vec3(0.f, 0.f, 0.f);
    float sum = 0.f;
    for(size_t j : particle.neighbors){
      particle.rho += W(particle.p - particles[j].p);
      ci += gradW(particle.p - particles[j].p);
      sum += norm(gradW(particle.p - particles[j].p)) * norm(gradW(particle.p - particles[j].p));
    }
    sum += norm(ci) * norm(ci);
    particle.rho *= sph_param.m;
    particle.lambda = - (particle.rho - sph_param.rho0) * sph_param.rho0 / sum;
  }
}

void scene_model::compute_dP(size_t i){
  for(size_t j : particles[i].neighbors){
    float s = - 0.1f * pow(W(particles[i].p - particles[j].p)/W(vcl::vec3(0.2f*sph_param.h, 0.f, 0.f)), 4.f);
    particles[i].dp += (particles[i].lambda + particles[j].lambda + s) * gradW(particles[i].p - particles[j].p);
  }
  particles[i].dp /= sph_param.rho0;
}

float distanceField(vcl::vec3 p){
  float d = fmin(1.f - p.x, p.x + 1.f);
  d = fmin(d, fmin(1.f - p.y, p.y + 1.f));
  d = fmin(d, fmin(1.f - p.z, p.z + 1.f));
  return d;
}

float sign(float x){
   return (x>=0.f) ? 1.f : -1.f;
}

void scene_model::solve_collision(size_t i, float dt){
  vcl::vec3 d = particles[i].q + particles[i].dp;
  if(d.x < -1.f){
      particles[i].v.x *= -1.f;
      particles[i].dp = vcl::vec3(0.f,0.f,0.f);
      particles[i].q = particles[i].p +  particles[i].v * dt ;
      particles[i].q.x = fmax(particles[i].q.x, -1.f);
  }
  if(d.x > 1.f){
      particles[i].v.x *= -1.f;
      particles[i].dp = vcl::vec3(0.f,0.f,0.f);
      particles[i].q = particles[i].p +  particles[i].v * dt ;
      particles[i].q.x = fmin(particles[i].q.x, 1.f);
  }
  if(d.y < -1.f){
      particles[i].v.y *= -1.f;
      particles[i].dp = vcl::vec3(0.f,0.f,0.f);
      particles[i].q = particles[i].p +  particles[i].v * dt ;
      particles[i].q.y = fmax(particles[i].q.y, -1.f);
  }
  if(d.y > 1.f){
      particles[i].v.y *= -1.f;
      particles[i].dp = vcl::vec3(0.f,0.f,0.f);
      particles[i].q = particles[i].p +  particles[i].v * dt ;
      particles[i].q.y = fmin(particles[i].q.y, 1.f);
  }
  if(d.z < -1.f){
      particles[i].v.z *= -1.f;
      particles[i].dp = vcl::vec3(0.f,0.f,0.f);
      particles[i].q = particles[i].p +  particles[i].v * dt ;
      particles[i].q.z = fmax(particles[i].q.z, -1.f);
  }
  if(d.z > 1.f){
      particles[i].v.z *= -1.f;
      particles[i].dp = vcl::vec3(0.f,0.f,0.f);
      particles[i].q = particles[i].p +  particles[i].v * dt ;
      particles[i].q.z = fmin(particles[i].q.z, 1.f);
  }
}

void scene_model::add_position_correction(){
  for(particle_element & particle : particles){
    particle.q += particle.dp;
    particle.dp = vcl::vec3(0.f, 0.f, 0.f);
  }
}

void scene_model::update_velocity(size_t i, float dt){
  particles[i].v = (particles[i].q - particles[i].p)/dt;
}

void scene_model::apply_vorticity(size_t i){    
    //A finir ! 


    for (auto & particle : particles)
    {
        vec3 w = {0,0,0};
        for (auto &&j : particle.neighbors)
        {
            vec3 v_ij = particles[j].v - particle.v;
            vec3 dw = gradW(particle.p - particles[j].p);
            w += cross(v_ij, dw);
        }   
    }
}

void scene_model::apply_viscosity(size_t i){

}

void scene_model::update_position(size_t i){
    particles[i].p = particles[i].q;
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

    const size_t N = particles.size();
    for(size_t k=0; k<N; ++k) {
        sphere.uniform.transform.translation = particles[k].p;
        draw(sphere, scene.camera);
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
