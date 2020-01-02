
#include "ffd.hpp"


#ifdef SCENE_FFD


using namespace vcl;

// A fonction to compute binomial coefficients
static int binomial_coeff(int n, int k);

void scene_model::deform_ffd()
{
    // Function to be completed in order to apply the FFD deformation on the shape
    // ...
    // (Note: you will possibly need to add variables and do precomputation outside of this function)
}


void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{

    // Generate the shape to be deformed (Uncomment the following lines to set another shape)
    shape = mesh_primitive_cylinder(0.5f, {0,0,-0.5f}, {0,0,0.5f}, 30, 15); // Deform a cylinder
    //  shape = mesh_primitive_parallelepiped_grid(10,10); // Deform a cubic grid
    //  shape = mesh_load_file_obj("scenes/sources/deformation/ffd/assets/bunny.obj"); // Deform a bunny

    // Initialize the visual representation of the shape
    visual = shape;
    visual.shader = shaders["mesh"];
    visual.uniform.color = {1,1,1};


    // Set the center of grid on the shape
    grid_center = average(shape.position);

    // Generate the 3D grid
    size_t3 const dimension = {4,4,4};            // Number of samples of the grid
    grid_length = {2,2,2};                        // Length of each size of the gris
    vec3 const p0 = grid_center-grid_length/2.0f; // Minimal position of the grid
    vec3 const p1 = grid_center+grid_length/2.0f; // Maximal position of the grid
    grid = linspace(p0, p1, dimension);           // Generate the coordinates of the grid vertices

    // Visual representation of the grid position
    sphere_grid = mesh_primitive_sphere(0.015f);
    sphere_grid.shader     = shaders["mesh"];

    // Used to draw grid segments
    segment_drawer.init();
    segment_drawer.uniform_parameter.color = {0,0,0};

    // Initial position of the camera
    scene.camera.orientation = rotation_from_axis_angle_mat3({1,0,0},3.14159f/2.0f);
    scene.camera.scale = 10.0f;



    // Set the time between to computation of the deformation
    timer_update_deform.periodic_event_time_step = 0.05f;



}





void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    set_gui();

    if(gui_scene.shape)      draw(visual, scene.camera);
    if(gui_scene.wireframe)  draw(visual, scene.camera, shaders["wireframe"]);

    if(gui_scene.grid)       display_grid(shaders, scene);


    // Call deform_ffd function if the deformation need to be updated
    if(is_deformed) {
        timer_update_deform.update();
        if(timer_update_deform.event) {
            deform_ffd();
        }
    }

}


void scene_model::mouse_move(scene_structure& scene, GLFWwindow* window)
{
    const vec2 cursor = glfw_cursor_coordinates_window(window);

    // Check that the mouse is clicked (drag and drop)
    const bool mouse_click_left  = glfw_mouse_pressed_left(window);
    const bool key_shift = glfw_key_shift_pressed(window);

    const size_t N = grid.data.size();

    // Selection
    if(!mouse_click_left && key_shift)
    {
        // Create the 3D ray passing by the selected point on the screen
        const ray r = picking_ray(scene.camera, cursor);

        // Check if this ray intersects a position (represented by a sphere)
        //  Loop over all positions and get the intersected position (the closest one in case of multiple intersection)

        picked_object = -1;
        float distance_min = 0.0f;
        for(size_t k=0; k<N; ++k)
        {
            const vec3& c = grid.data[k];
            const picking_info info = ray_intersect_sphere(r, c, 0.05f);

            if( info.picking_valid ) // the ray intersects a sphere
            {
                const float distance = norm(info.intersection-r.p); // get the closest intersection
                if( picked_object==-1 || distance<distance_min ){
                    picked_object = k;
                    distance_min = distance;
                }
            }

        }
    }

    // Displacement
    if(mouse_click_left && key_shift && picked_object!=-1)
    {
        // Translate the selected object to the new pointed mouse position within the camera plane
        // ************************************************************************************** //

        // Get vector orthogonal to camera orientation
        const mat4 M = scene.camera.camera_matrix();
        const vec3 n = {M(0,2),M(1,2),M(2,2)};

        // Compute intersection between current ray and the plane orthogonal to the view direction and passing by the selected object
        const ray r = picking_ray(scene.camera, cursor);
        vec3& p0 = grid.data[picked_object];
        const picking_info info = ray_intersect_plane(r,n,p0);

        // Deform the mesh
        is_deformed = true;

        // translate the position
        p0 = info.intersection;
    }
}

void scene_model::display_grid(std::map<std::string,GLuint>& shaders, scene_structure& scene)
{
    int N = grid.data.size();

    // Display spheres
    for(int k=0; k<N; ++k) {
        sphere_grid.uniform.transform.translation = grid.data[k];
        if(k == picked_object) sphere_grid.uniform.color = {1,0,0};
        else                   sphere_grid.uniform.color = {0,0,1};
        draw(sphere_grid, scene.camera);
    }


    // Display edges
    for(int kx=0; kx<int(grid.dimension[0]); ++kx) {
        for(int ky=0; ky<int(grid.dimension[1]); ++ky) {
            for(int kz=0; kz<int(grid.dimension[2]); ++kz) {

                if(kx-1>=0){
                    segment_drawer.uniform_parameter.p1 = grid(kx-1,ky,kz);
                    segment_drawer.uniform_parameter.p2 = grid(kx ,ky,kz);
                    segment_drawer.draw(shaders["segment_im"], scene.camera);
                }

                if(ky-1>=0){
                    segment_drawer.uniform_parameter.p1 = grid(kx,ky-1,kz);
                    segment_drawer.uniform_parameter.p2 = grid(kx,ky  ,kz);
                    segment_drawer.draw(shaders["segment_im"], scene.camera);
                }

                if(kz-1>=0){
                    segment_drawer.uniform_parameter.p1 = grid(kx,ky,kz-1);
                    segment_drawer.uniform_parameter.p2 = grid(kx,ky,kz  );
                    segment_drawer.draw(shaders["segment_im"], scene.camera);
                }

            }
        }
    }
}


int binomial_coeff(int n, int k)
{
    int res = 1;
    if(k>n-k)
        k = n - k;
    for(int i=0; i<k; ++i) {
        res *= (n-i);
        res /= (i+1);
    }
    return res;
}



void scene_model::set_gui()
{
    ImGui::Checkbox("Wireframe", &gui_scene.wireframe); ImGui::SameLine();
    ImGui::Checkbox("Shape", &gui_scene.shape);
    ImGui::Checkbox("Grid", &gui_scene.grid);
}



#endif

