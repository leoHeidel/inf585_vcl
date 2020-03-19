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
    std::default_random_engine generator;
    std::normal_distribution<float> normal(0,1);

    sph_param.m = sph_param.rho0*sph_param.h*sph_param.h*sph_param.h;

    for (size_t i = 0; i < sph_param.nb_particles; i++)
    {
       vec3 v = {normal(generator), normal(generator), normal(generator)};
       particle_element particle;
       particle.p = 0.3*v;
       particles.push_back(particle);
    }

    oclHelper.init_context(sph_param);
}

void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    count++;
    if (count > 50) {
        // const float dt = timer.update();
        set_gui();

        // Force constant time step
        size_t solverIterations = 3;
        size_t subSteps = 1;
        for(size_t t=0; t<subSteps; ++t){
          std::vector<vec3> v;
          for (auto &part : particles){
              v.push_back(part.v);
          }

          std::vector<vec3> positions;
          for (auto &part : particles){
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
          for (size_t i = 0; i < particles.size(); i++){
              particles[i].p = p_gpu[i];
          }

          std::vector<vcl::vec3> v_gpu = oclHelper.get_v();
          for (size_t i = 0; i < particles.size(); i++){
              particles[i].v = v_gpu[i];
          }
        }
    }
    display(shaders, scene, gui);
}

void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& , gui_structure& gui)
{
    gui.show_frame_camera = false;

    billboard = mesh_drawable( mesh_primitive_quad());
    std::vector<vec3> borders_segments = {{-1,-1,-1},{1,-1,-1}, {1,-1,-1},{1,1,-1}, {1,1,-1},{-1,1,-1}, {-1,1,-1},{-1,-1,-1},
                                          {-1,-1,1} ,{1,-1,1},  {1,-1,1}, {1,1,1},  {1,1,1}, {-1,1,1},  {-1,1,1}, {-1,-1,1},
                                          {-1,-1,-1},{-1,-1,1}, {1,-1,-1},{1,-1,1}, {1,1,-1},{1,1,1},   {-1,1,-1},{-1,1,1}};
    borders = segments_gpu(borders_segments);
    borders.uniform.color = {0,0,0};
    borders.shader = shaders["curve"];

    initialize_sph();

    gui_param.display_field = true;
    gui_param.display_particles = true;
    gui_param.save_field = false;
}

void scene_model::display(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    draw(borders, scene.camera);

    GLuint shader = shaders["fluid"];
    glUseProgram(shader);
    uniform(shader, "rotation", scene.camera.orientation); //opengl_debug();
    uniform(shader, "scaling", sph_param.h / 3 *2); //opengl_debug();
    uniform(shader,"perspective",scene.camera.perspective.matrix()); //opengl_debug();
    uniform(shader,"view",scene.camera.view_matrix()); //opengl_debug();
    uniform(shader,"camera_position",scene.camera.camera_position()); //opengl_debug();
    for(size_t k=0; k<particles.size(); ++k) {
      uniform(shader, "translation", particles[k].p); //opengl_debug();
      vcl::draw(billboard.data); //opengl_debug();
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
