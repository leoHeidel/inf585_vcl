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
        size_t solverIterations = 5;
        auto last_time = std::chrono::high_resolution_clock::now();

        // Setting gravity direction depending on option
        if(gui_param.world_space_gravity){
          sph_param.gx = (scene.camera.orientation*vec3(0.0f, -100.0*sph_param.h, 0.0f)).x;
          sph_param.gy = (scene.camera.orientation*vec3(0.0f, -100.0*sph_param.h, 0.0f)).y;
          sph_param.gz = (scene.camera.orientation*vec3(0.0f, -100.0*sph_param.h, 0.0f)).z;
          oclHelper.set_sph_param(sph_param);
        }else{
          sph_param.gx = (vec3(0.0f, -100.0*sph_param.h, 0.0f)).x;
          sph_param.gy = (vec3(0.0f, -100.0*sph_param.h, 0.0f)).y;
          sph_param.gz = (vec3(0.0f, -100.0*sph_param.h, 0.0f)).z;
          oclHelper.set_sph_param(sph_param);
        }

        // Update position and velocity
        oclHelper.befor_solver();

        auto current_time = std::chrono::high_resolution_clock::now();
        pre_solver_time = alpha_time*pre_solver_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(current_time-last_time).count();
        last_time = current_time;

        // Find particle neighbors
        oclHelper.make_neighboors();
        size_t k=0;

        current_time = std::chrono::high_resolution_clock::now();
        neighboors_time = alpha_time*neighboors_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(current_time-last_time).count();
        last_time = current_time;

        // Iteration loop adding constraints
        while(k<solverIterations){
          oclHelper.solver_step();
          ++k;
        }

        current_time = std::chrono::high_resolution_clock::now();
        solver_time = alpha_time*solver_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(current_time-last_time).count();
        last_time = current_time;

        // Re-update speed (apply vorticity and viscosity)
        oclHelper.update_speed();

        std::vector<vcl::vec3> p_gpu = oclHelper.get_p();
        for (size_t i = 0; i < particles.size(); i++)
        {
            particles[i].p = p_gpu[i];
        }
        current_time = std::chrono::high_resolution_clock::now();
        post_solver_time = alpha_time*post_solver_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(current_time-last_time).count();
        last_time = current_time;

        oclHelper.log_pressure();
    }

    // Render the fluid
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

    // One billboard per particle
    billboard = mesh_drawable( mesh_primitive_quad());

    std::vector<vec3> borders_segments = {{-1,-1,-1},{1,-1,-1}, {1,-1,-1},{1,1,-1}, {1,1,-1},{-1,1,-1}, {-1,1,-1},{-1,-1,-1},
                                          {-1,-1,1} ,{1,-1,1},  {1,-1,1}, {1,1,1},  {1,1,1}, {-1,1,1},  {-1,1,1}, {-1,-1,1},
                                          {-1,-1,-1},{-1,-1,1}, {1,-1,-1},{1,-1,1}, {1,1,-1},{1,1,1},   {-1,1,-1},{-1,1,1}};
    borders = segments_gpu(borders_segments);
    borders.uniform.color = {0,0,0};
    borders.shader = shaders["curve"];

    cube = mesh_drawable(mesh_primitive_quad());
    cube.uniform.color = {1,0,0};
    cube.shader = shaders["fluid_box"];

    initialize_sph();

    gui_param.display_field = true;
    gui_param.display_particles = true;
    gui_param.save_field = false;
    gui_param.world_space_gravity = false;
    gui_param.advanced_shading = false;
    gui_param.more_advanced_shading = false;

    //Initializing render target framebuffers
    oglHelper.initializeFBO(dfbo, true);
    oglHelper.initializeFBO(rfbo, true);
    oglHelper.initializeFBO(sdfbo, true);
    oglHelper.initializeFBO(srfbo, true);
    oglHelper.initializeFBO(bgfbo, false);

    //Set the texture to be shown on screen
    screenquad = mesh_drawable( mesh_primitive_quad(vec3(-1,-1,0),vec3(1,-1,0),vec3(1,1,0),vec3(-1,1,0)));
    screenquad.shader = shaders["render_target"];
}

// Render fluid
void scene_model::display(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    if(!gui_param.advanced_shading){
      gui_param.more_advanced_shading = false;
      basic_render(shaders["basic_fluid"], scene);
    }else if(gui_param.advanced_shading && !gui_param.more_advanced_shading){
      //draw particles depth/reverse depth to render target textures
      draw_depth_buffer(shaders["depth"], dfbo, scene); //draw particles' depth to buffer dfbo
      draw_thickness_buffer(shaders["thickness"], rfbo, scene); //draw particles' reverse depth to buffer rfbo

      //give depth textures to blur shader (rendering to render target sdfbo)
      draw_blur_buffer(shaders["blur"], dfbo, sdfbo, screenquad, false); // store blurred depth in sdfbo
      draw_blur_buffer(shaders["blur"], rfbo, srfbo, screenquad, true); // store blurred reverse depth in srfbo

      //give depth texture and reverse depth texture to screenquad's shader (rendering a quad on screen)
      render_cube(cube.shader, 0, scene, true, false);
      render_to_screen(scene);
      render_cube(cube.shader, 0, scene, false, false);
    }else if(gui_param.advanced_shading && gui_param.more_advanced_shading){
      //draw particles depth/reverse depth to render target textures
      draw_depth_buffer(shaders["depth"], dfbo, scene); //draw particles' depth to buffer dfbo
      draw_thickness_buffer(shaders["thickness"], rfbo, scene); //draw particles' reverse depth to buffer rfbo

      //give depth textures to blur shader (rendering to render target sdfbo)
      draw_blur_buffer(shaders["blur"], dfbo, sdfbo, screenquad, false); // store blurred depth in sdfbo
      draw_blur_buffer(shaders["blur"], rfbo, srfbo, screenquad, true); // store blurred reverse depth in srfbo

      //give depth texture and reverse depth texture to screenquad's shader (rendering a quad on screen)
      render_cube(cube.shader, bgfbo[0], scene, true, true);
      draw_deformed_background(shaders["deform_background"], scene); //background refraction
      render_to_screen(scene);
      render_cube(cube.shader, 0, scene, false, true);
    }else{

    }
}

// Handles the rendering of the cube's edges when Advanced shading option on
void scene_model::render_cube(GLuint shader, GLuint id, scene_structure& scene, bool isBack, bool isChecker){
  const float pi = 3.14159265f;
  glUseProgram(shader);
  uniform(shader, "isBack", isBack); //opengl_debug();
  uniform(shader, "isChecker", isChecker); //opengl_debug();
  uniform(shader, "scaling", 2.0f);
  uniform(shader,"perspective",scene.camera.perspective.matrix()); //opengl_debug();
  uniform(shader,"view",scene.camera.view_matrix()); opengl_debug();
  uniform(shader,"camera_position",scene.camera.camera_position()); //opengl_debug();
  glBindVertexArray(cube.data.vao); //opengl_debug();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.data.vbo_index); //opengl_debug();
  glBindFramebuffer(GL_FRAMEBUFFER, id);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClearDepth(1.0);
  if(isBack){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_DEPTH_TEST);
  uniform(shader, "translation", {1, 0, 0});
  uniform(shader, "rotation", rotation_from_axis_angle_mat3({0, 1, 0}, pi/2.0f));
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug();
  uniform(shader, "translation", vec3(-1, 0, 0));
  uniform(shader, "rotation", rotation_from_axis_angle_mat3({0, 1, 0}, -pi/2.0f));
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug();
  uniform(shader, "translation", vec3(0, 1, 0));
  uniform(shader, "rotation", rotation_from_axis_angle_mat3({1, 0, 0}, -pi/2));
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug();
  uniform(shader, "translation", vec3(0, -1, 0));
  uniform(shader, "rotation", rotation_from_axis_angle_mat3({1, 0, 0}, pi/2));
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug();
  uniform(shader, "translation", vec3(0, 0, 1));
  uniform(shader, "rotation", rotation_from_axis_angle_mat3({0, 1, 0}, 0));
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug();
  uniform(shader, "translation", vec3(0, 0, -1));
  uniform(shader, "rotation", rotation_from_axis_angle_mat3({0, 1, 0}, pi));
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug();
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //opengl_debug();
  glBindVertexArray(0);
}

// Draw deformed background to screen
void scene_model::draw_deformed_background(GLuint shader, scene_structure& scene){
  glUseProgram(shader);
  glUniform1i(glGetUniformLocation(shader, "thickness_tex"), 0);
  glUniform1i(glGetUniformLocation(shader, "background_tex"), 1);
  glUniform1i(glGetUniformLocation(shader, "depth_tex"), 2);
  glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
  glBindTexture(GL_TEXTURE_2D, srfbo[1]);
  glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 1
  glBindTexture(GL_TEXTURE_2D, bgfbo[1]);
  glActiveTexture(GL_TEXTURE0 + 2); // Texture unit 2
  glBindTexture(GL_TEXTURE_2D, sdfbo[1]);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindVertexArray(screenquad.data.vao); opengl_debug();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screenquad.data.vbo_index); opengl_debug();
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); opengl_debug();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); opengl_debug();
  glBindVertexArray(0);
  glDisable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, 0);
}

// Basic render
void scene_model::basic_render(GLuint shader, scene_structure& scene){
  glClearDepth(1.0);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw(borders, scene.camera);
  glUseProgram(shader);
  uniform(shader, "rotation", scene.camera.orientation); opengl_debug();
  uniform(shader, "scaling", sph_param.h * 2 / 3); opengl_debug();
  uniform(shader,"perspective",scene.camera.perspective.matrix()); opengl_debug();
  uniform(shader,"view",scene.camera.view_matrix()); opengl_debug();
  uniform(shader,"camera_position",scene.camera.camera_position()); opengl_debug();
  uniform(shader,"radius",sph_param.h); opengl_debug();
  glBindVertexArray(billboard.data.vao); opengl_debug();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboard.data.vbo_index); opengl_debug();
  glBindFramebuffer(GL_FRAMEBUFFER, 0); opengl_debug();
  glEnable(GL_DEPTH_TEST); opengl_debug();
  glDepthFunc(GL_LESS); opengl_debug();
  for(size_t k=0; k<particles.size(); ++k) {
    uniform(shader, "translation", particles[k].p); opengl_debug();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); opengl_debug();
  }
  glDepthFunc(GL_LESS);
  glDisable(GL_DEPTH_TEST);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); opengl_debug();
  glBindVertexArray(0);
}

// Draw particle's depth/reverse depth to buffer fbo
void scene_model::draw_depth_buffer(GLuint shader, GLuint fbo[3], scene_structure& scene){
  glUseProgram(shader);
  uniform(shader, "rotation", scene.camera.orientation); //opengl_debug();
  uniform(shader, "scaling", sph_param.h); //opengl_debug();
  uniform(shader,"perspective",scene.camera.perspective.matrix()); //opengl_debug();
  uniform(shader,"view",scene.camera.view_matrix()); //opengl_debug();
  uniform(shader,"camera_position",scene.camera.camera_position()); //opengl_debug();
  uniform(shader,"radius",sph_param.h); //opengl_debug();
  glBindVertexArray(billboard.data.vao); //opengl_debug();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboard.data.vbo_index); //opengl_debug();
  glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
  glClearDepth(1.0f);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
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

void scene_model::draw_thickness_buffer(GLuint shader, GLuint fbo[3], scene_structure& scene){
  glUseProgram(shader);
  uniform(shader, "rotation", scene.camera.orientation); //opengl_debug();
  uniform(shader, "scaling", sph_param.h); //opengl_debug();
  uniform(shader,"perspective",scene.camera.perspective.matrix()); //opengl_debug();
  uniform(shader,"view",scene.camera.view_matrix()); //opengl_debug();
  uniform(shader,"camera_position",scene.camera.camera_position()); //opengl_debug();
  uniform(shader,"radius",sph_param.h); //opengl_debug();
  glBindVertexArray(billboard.data.vao); //opengl_debug();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboard.data.vbo_index); //opengl_debug();
  glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
  glClearDepth(1.0f);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for(size_t k=0; k<particles.size(); ++k) {
    uniform(shader, "translation", particles[k].p); //opengl_debug();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug();
  }
  glDisable(GL_BLEND);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //opengl_debug();
  glBindVertexArray(0);
}

// Blur source buffer and render to target buffer (bilateral gaussian blur)
void scene_model::draw_blur_buffer(GLuint shader, GLuint source[3], GLuint target[3], mesh_drawable quad, bool isThickness){
  glUseProgram(shader);
  uniform(shader, "isThickness", isThickness); //opengl_debug();
  glBindVertexArray(quad.data.vao); //opengl_debug();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.data.vbo_index); //opengl_debug();

  glBindFramebuffer(GL_FRAMEBUFFER, target[0]); //drawing to render target sdfbo
  glUniform1i(glGetUniformLocation(shader, "depth_tex_sampler"), 1);
  glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 0
  glBindTexture(GL_TEXTURE_2D, source[1]); //sending dfbo[1] as uniform sampler2D to shader
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); //opengl_debug(); //draw quad to render target
  glDisable(GL_BLEND);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //opengl_debug();
  glBindVertexArray(0);
}

// Render result to screen
void scene_model::render_to_screen(scene_structure& scene){
  //draw(borders, scene.camera);
  GLuint shader = screenquad.shader; // = shaders['render target']
  glUseProgram(shader);
  uniform(shader, "rotation", scene.camera.orientation); //opengl_debug();
  uniform(shader, "view", scene.camera.view_matrix()); //opengl_debug();
  uniform(shader,"perspective",scene.camera.perspective.matrix()); //opengl_debug();
  uniform(shader,"camera_position",scene.camera.camera_position()); //opengl_debug();
  glUniform1i(glGetUniformLocation(shader, "depth_tex_sampler"), 1);
  glUniform1i(glGetUniformLocation(shader, "rev_depth_tex_sampler"), 2);
  glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 0
  glBindTexture(GL_TEXTURE_2D, sdfbo[1]);
  glActiveTexture(GL_TEXTURE0 + 2); // Texture unit 1
  glBindTexture(GL_TEXTURE_2D, srfbo[1]);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  draw(screenquad, scene.camera);
  glDisable(GL_BLEND);
  //draw(borders, scene.camera);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void scene_model::set_gui()
{
    float dt_min = 0.01f, dt_max = 0.05f;
    ImGui::SliderScalar("dt", ImGuiDataType_Float, &sph_param.dt, &dt_min, &dt_max, "%.3f");
    float h_min = 0.01f, h_max = 0.1f;
    ImGui::SliderScalar("h", ImGuiDataType_Float, &sph_param.h, &h_min, &h_max, "%.3f");
    float m_min = 0.1f, m_max = 5.f;
    ImGui::SliderScalar("m", ImGuiDataType_Float, &sph_param.m, &m_min, &m_max, "%.3f");
    float c_min = 0.05f, c_max = 0.5f;
    ImGui::SliderScalar("viscuosity", ImGuiDataType_Float, &sph_param.c, &c_min, &c_max, "%.3f");

    ImGui::Checkbox("World Space Gravity", &gui_param.world_space_gravity);
    ImGui::Checkbox("Advanced Shading", &gui_param.advanced_shading);
    if(gui_param.advanced_shading){
      ImGui::Checkbox("with Background refraction", &gui_param.more_advanced_shading);
    }
}

#endif
