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

    sph_param.m = sph_param.rho0*sph_param.h*sph_param.h*sph_param.h * 2;

    for (size_t i = 0; i < sph_param.nb_particles; i++)
    {
       vec3 v = {normal(generator), normal(generator), normal(generator)};
       particle_element particle;
       particle.p = 0.3*v;
       particles.push_back(particle);
    }

    oclHelper.init_context(sph_param);

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
    oclHelper.set_p_v(positions,v);
}



void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    auto start_func = std::chrono::high_resolution_clock::now();
    count++;
    if (count > 50) {
        set_gui();

        // Force constant time step
        size_t solverIterations = 3;
        auto last_time = std::chrono::high_resolution_clock::now();

        oclHelper.befor_solver();

        auto current_time = std::chrono::high_resolution_clock::now();
        pre_solver_time = alpha_time*pre_solver_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(current_time-last_time).count();
        last_time = current_time;

        oclHelper.make_neighboors();
        size_t k=0;

        current_time = std::chrono::high_resolution_clock::now();
        neighboors_time = alpha_time*neighboors_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(current_time-last_time).count();
        last_time = current_time;

        while(k<solverIterations){
        oclHelper.solver_step();
        ++k;
        }

        current_time = std::chrono::high_resolution_clock::now();
        solver_time = alpha_time*solver_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(current_time-last_time).count();
        last_time = current_time;

        oclHelper.update_speed();

        std::vector<vcl::vec3> p_gpu = oclHelper.get_p();
        for (size_t i = 0; i < particles.size(); i++)
        {
            particles[i].p = p_gpu[i];
        }
        current_time = std::chrono::high_resolution_clock::now();
        post_solver_time = alpha_time*post_solver_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(current_time-last_time).count();
        last_time = current_time;
    }

    auto befor_display = std::chrono::high_resolution_clock::now();
    display(shaders, scene, gui);
    auto after_dislplay = std::chrono::high_resolution_clock::now();
    render_time = alpha_time*render_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(after_dislplay-befor_display).count();

    auto end_func = std::chrono::high_resolution_clock::now();
    total_time = alpha_time*total_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(end_func - start_func).count();

    if (! ((count + 1) % 100)) {
        std::cout << "pre solver time: " << pre_solver_time << std::endl;
        std::cout << "neigbors time: " << neighboors_time << std::endl;
        std::cout << "neigbors sub times: " << oclHelper.nn1_time << " " << oclHelper.nn2_time << " " << oclHelper.nn3_time << std::endl;
        std::cout << "solver time: " << solver_time << std::endl;
        std::cout << "post solver time: " << post_solver_time << std::endl;
        std::cout << "render time: " << render_time << std::endl;
        std::cout << "total time: " << total_time << std::endl;
        std::cout << std::endl;
    }
}

void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
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

    //Initializing depth render target framebuffer
    oglHelper.initializeFBO(dfbo);
    oglHelper.initializeFBO(rfbo);

    //Set the texture to be shown on screen
    screenquad = mesh_drawable( mesh_primitive_quad(vec3(-1,-1,0),vec3(1,-1,0),vec3(1,1,0),vec3(-1,1,0)));
    screenquad.shader = shaders["render_target"];
    //screenquad.texture_id = rfbo[1];
}

void scene_model::drawOn(GLuint buffer_id, GLuint shader, bool reverseDepth = false){
  //glUseProgram(shader);
  glBindVertexArray(billboard.data.vao); //opengl_debug();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboard.data.vbo_index); //opengl_debug();
  glBindFramebuffer(GL_FRAMEBUFFER, buffer_id);
  if(reverseDepth){
    glClearDepth(0.0f);
  }else{
    glClearDepth(1.0f);
  }
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  if(reverseDepth){
    glDepthFunc(GL_GREATER);
  }else{
    glDepthFunc(GL_LESS);
  }
  for(size_t k=0; k<particles.size(); ++k) {
    uniform(shader, "translation", particles[k].p); //opengl_debug();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug();
  }
  glDepthFunc(GL_LESS);
  glDisable(GL_DEPTH_TEST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //opengl_debug();
  glBindVertexArray(0);
}

void scene_model::display(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    GLuint shader = shaders["fluid"];
    glUseProgram(shader);
    uniform(shader, "rotation", scene.camera.orientation); //opengl_debug();
    uniform(shader, "scaling", sph_param.h / 3 *2); //opengl_debug();
    uniform(shader,"perspective",scene.camera.perspective.matrix()); //opengl_debug();
    uniform(shader,"view",scene.camera.view_matrix()); //opengl_debug();
    uniform(shader,"camera_position",scene.camera.camera_position()); //opengl_debug();
    drawOn(dfbo[0], shader, false);
    drawOn(rfbo[0], shader, true);

    draw(borders, scene.camera);
    glUseProgram(screenquad.shader);
    glUniform1i(glGetUniformLocation(screenquad.shader, "depth_tex_sampler"), 0);
    glUniform1i(glGetUniformLocation(screenquad.shader, "rev_depth_tex_sampler"), 1);
    glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
    glBindTexture(GL_TEXTURE_2D, dfbo[1]);
    glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 1
    glBindTexture(GL_TEXTURE_2D, rfbo[1]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(screenquad, scene.camera);
    glDisable(GL_BLEND);
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
