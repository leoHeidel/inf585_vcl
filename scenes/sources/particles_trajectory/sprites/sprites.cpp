
#include "sprites.hpp"

#include <random>

#ifdef SCENE_PARTICLES_TRAJECTORY_SPRITES

using namespace vcl;



void scene_model::load_decoration_meshes(GLuint shader)
{
    cooking_pot                = mesh_load_file_obj("scenes/sources/particles_trajectory/sprites/assets/cauldron.obj");
    cooking_pot.shader         = shader;
    cooking_pot.uniform.color  = {0.9f, 0.8f, 0.6f};
    cooking_pot.uniform.transform.translation = {-0.1f, -0.3f, 0};
    cooking_pot.uniform.transform.scaling     = 0.43f;

    spoon               = mesh_load_file_obj("scenes/sources/particles_trajectory/sprites/assets/spoon.obj");
    spoon.shader        = shader;
    spoon.uniform.color = {0.9f, 0.8f, 0.6f};
    spoon.uniform.transform.translation = {-0.1f, -0.3f, 0};
    spoon.uniform.transform.scaling     = 0.43f;

}


/** This function is called before the beginning of the animation loop
    It is used to initialize all part-specific data */
void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    load_decoration_meshes(shaders["mesh"]);

    bubble = mesh_primitive_sphere(0.03f);
    bubble.shader = shaders["mesh"];

    sprite = mesh_primitive_quad();
    sprite.shader = shaders["mesh"];
    sprite.uniform.color = {1,1,1};
    sprite.uniform.shading = {1,0,0};
    sprite.uniform.transform.scaling = 0.75f;
    sprite.texture_id = create_texture_gpu( image_load_png("scenes/sources/particles_trajectory/sprites/assets/smoke.png") );


    scene.camera.scale = 5.0f;
    scene.camera.orientation = rotation_from_axis_angle_mat3({0,1,0},3.14f/4.0f)*rotation_from_axis_angle_mat3({1,0,0},-3.14f/8.0f);
    gui.show_frame_worldspace = true;
    gui.show_frame_camera = false;

    timer_bubble.periodic_event_time_step = 0.25f;
    timer_sprite.periodic_event_time_step = 0.5f;
}

particle_structure create_new_bubble(float time)
{
    // Code to be filled and adapted appropriately
    //
    // Basic initialization ...
    particle_structure particle;
    particle.t0 = time;
    particle.p0 = {0,0,0};

    return particle;
}
particle_structure create_new_sprite(float time)
{
    // Code to be filled and adapted appropriately
    //
    // Basic initialization ...
    particle_structure particle;
    particle.t0 = time;
    particle.p0 = {0,0,0};

    return particle;
}

/** This function is called at each frame of the animation loop.
    It is used to compute time-varying argument and perform data data drawing */
void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    set_gui();

    timer_bubble.update();
    float const time = timer_bubble.t;

    // Display decoration
    if(gui_scene.display_decoration){
        draw(cooking_pot, scene.camera);
        draw(spoon, scene.camera);
    }

    // If a new bubble need to be created
    if( timer_bubble.event )
        particles_bubble.push_back(create_new_bubble(time));

    // Compute trajectory and display the bubbles
    for(particle_structure& particle : particles_bubble)
    {
        // Correct and adapt this code ...
        vec3 const p = particle.p0 + vec3(0,time-particle.t0,0);
        bubble.uniform.transform.translation = p;

        draw(bubble, scene.camera);
        particle.trajectory.add_point(p);
    }


    // If a new sprite need to be created
    timer_sprite.update();
    if( timer_sprite.event )
        particles_sprite.push_back( create_new_sprite(time) );

    // Compute trajectory and display the sprites
    for(particle_structure& particle : particles_sprite)
    {
        // Correct and adapt this code ...
        //

        // Prepare for transparent element drawing
        glEnable(GL_BLEND);
        glDepthMask(false); // deactivate zbuffer writing
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        vec3 const p = particle.p0 + vec3(0,0,time-particle.t0);
        sprite.uniform.transform.translation = p;

        draw(sprite, scene.camera);
        particle.trajectory.add_point(p);
        glDepthMask(true); // Important: reactivate writing in ZBuffer for future drawing
    }

    // Do not forget to erase particles after some time

}



void scene_model::set_gui()
{
    // Start/Stop animation
    const bool start = ImGui::Button("Start"); ImGui::SameLine();
    const bool stop  = ImGui::Button("Stop");
    if(start) {timer_bubble.start(); timer_sprite.start();}
    if(stop) {timer_bubble.stop(); timer_sprite.stop();}

    // Speed of the animation
    ImGui::SliderFloat("Time scale", &timer_bubble.scale, 0.05f, 2.0f);
    timer_sprite.scale = timer_bubble.scale;

    // Interval of time between creation of a new particle
    ImGui::SliderFloat("Timer event bubbles", &timer_bubble.periodic_event_time_step, 0.01f, 0.5f);
    ImGui::SliderFloat("Timer event sprite", &timer_sprite.periodic_event_time_step, 0.01f, 0.5f);

    ImGui::Checkbox("Display decoration",&gui_scene.display_decoration); ImGui::SameLine();
}



#endif
