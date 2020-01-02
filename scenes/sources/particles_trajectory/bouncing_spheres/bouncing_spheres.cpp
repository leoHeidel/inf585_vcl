
#include "bouncing_spheres.hpp"

#include <random>

#ifdef SCENE_PARTICLES_TRAJECTORY_BOUNCING_SPHERES

using namespace vcl;



/** This function is called before the beginning of the animation loop
    It is used to initialize all part-specific data */
void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    // Setup the ground mesh as a disc
    ground = mesh_primitive_disc(1.25f,{0,-0.01f,0},{0,1,0},80);
    ground.uniform.color = vec3(1,1,1);
    ground.shader = shaders["mesh"];

    // Setup the sphere mesh used to display the particles
    sphere = mesh_drawable(mesh_primitive_sphere(0.03f));
    sphere.uniform.color = vec3(0.6f, 0.6f, 1.0f);
    sphere.shader = shaders["mesh"];
    // Setup time between each creation of a new particle
    timer.periodic_event_time_step = 0.2f;

    // Setup initial configuration of the camera
    scene.camera.scale = 5.0f;
    scene.camera.orientation = rotation_from_axis_angle_mat3({0,1,0},3.14f/4.0f)*rotation_from_axis_angle_mat3({1,0,0},-3.14f/8.0f);
    // Initial configuration of the parameters in the GUI
    gui.show_frame_worldspace = true;
    gui.show_frame_camera     = false;
}


particle_structure create_new_particle(float time)
{
    // Set properties of the new particle
    particle_structure particle;
    particle.t0 = time;
    particle.p0 = {0,0,0};                        // Initial position
    float const theta = rand_interval(0,2*3.14f); // Initial angle
    particle.v0 = {std::cos(theta), 4.5f+rand_interval(), std::sin(theta)}; // Initial speed

    // Follow trajectory
    particle.trajectory = curve_dynamic_drawable(10);
    particle.trajectory.uniform.color = vec3{0,0,1};

    return particle;
}

/** This function is called at each frame of the animation loop.
    It is used to compute time-varying argument and perform data data drawing */
void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    // Update the time
    timer.update();
    set_gui();

    // retrieve current time
    float const t_current = timer.t;

    // Indicate if a new particle should be created
    bool const is_new_particle = timer.event;
    if( is_new_particle ) {
        particle_structure const new_particle = create_new_particle(t_current);
        particles.push_back(new_particle);
    }


    // Compute new position
    vec3 const g = {0,-9.81f,0};
    for(particle_structure& particle : particles)
    {
        float const t = t_current-particle.t0;                    // Local time with respect to particle
        vec3 const p  = g*t*t/2.0f + particle.v0*t + particle.p0; // Trajectory

        sphere.uniform.transform.translation = p;    // Update the position of particle
        draw(sphere, scene.camera, shaders["mesh"]); // Display the particle

        // Update and display the trajectory following the particle
        particle.trajectory.add_point(p);
        particle.trajectory.draw(shaders["curve"], scene.camera);


    }

    // Display ground
    draw(ground, scene.camera, shaders["mesh"]);

    // Remove old particles
    for(auto it = particles.begin(); it!=particles.end(); ++it)
        if( t_current-it->t0 > 2)
            it = particles.erase(it);
}


/** Part specific GUI drawing */
void scene_model::set_gui()
{
    // Start/Stop animation
    const bool start = ImGui::Button("Start"); ImGui::SameLine();
    const bool stop  = ImGui::Button("Stop");
    if(start) timer.start();
    if(stop) timer.stop();

    // Speed of the animation
    ImGui::SliderFloat("Time scale", &timer.scale, 0.05f, 2.0f);

    // Interval of time between creation of a new particle
    ImGui::SliderFloat("Timer event", &timer.periodic_event_time_step, 0.01f, 0.5f);
}



#endif
