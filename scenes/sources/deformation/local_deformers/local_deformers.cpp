
#include "local_deformers.hpp"



#include <set>

#ifdef SCENE_LOCAL_DEFORMER


using namespace vcl;


// Function called every time a deformation is applied
//  i.e. When a vertex is picked and displaced
// The function receives as parameter:
//  - The current state of the camera
//  - The translation currently applied in the 2D window coordinates
void scene_model::update_deformation(const vcl::camera_scene& camera, const vec2& t)
{
    /** TO DO
     *  Update this function to implement the expected deformations
     */
    const float r = gui_scene.falloff;   // the falloff distance
    const int N = shape.position.size(); // the number of vertices

    // Convert the 2D translation in screen coordinates into 3D translation within the plane oriented with respect to the camera
    const vec3 translation = camera.orientation * vec3(t.x, t.y, 0);
    for(int k=0; k<N; ++k)
    {
        vec3& p = shape.position[k];        // the current position of the selected vertex
        const vec3& p0 = position_saved[k]; // the initial position of the selected vertex (stored as long as vertex is dragged)

        // Distance between current vertex and picked one
        const float dist = norm(position_saved[k]-picking.p0);

        // The following deformation should be modified
        if(dist<r)
            p = p0 + translation;  // apply a translation when the distance is less than the falloff radius
    }
    require_normal_update = true;  // note that the surface normals should be updated (updated only periodically to avoid extra computation)

}



void scene_model::setup_data(std::map<std::string,GLuint>& shaders_arg, scene_structure& , gui_structure& )
{
    // Store shaders as class attribute
    shaders = shaders_arg;

    // Initialize the current shape to a plane
    initialize_plane();

    // Vertex picking displayed as a sphere
    sphere_picking = mesh_primitive_sphere(0.01f);
    sphere_picking.shader = shaders["mesh"];
    picking.selected = false;

    // Initialize falloff distance
    gui_scene.falloff = 0.3f;
    // Display falloff distance as a circle
    falloff_circle = curve_primitve_circle();
    falloff_circle.shader = shaders["curve"];
    falloff_circle.uniform.color = {1,0,0};

    // Timer to update normals
    timer.periodic_event_time_step = 0.20f;
}



void scene_model::frame_draw(std::map<std::string,GLuint>& , scene_structure& scene, gui_structure& )
{

    set_gui();

    // Update normals periodically when the mesh is deformed
    timer.update();
    if(timer.event && require_normal_update){
        normal(shape.position, shape.connectivity, shape.normal);
        visual.update_normal(shape.normal);
        require_normal_update = false;
    }


    // Display the shape
    draw(visual, scene.camera);
    if(gui_scene.wireframe)
        draw(visual, scene.camera, shaders["wireframe_quads"]);
    if(gui_scene.normals)
        draw(visual, scene.camera, shaders["normals"]);


    // Display visual feedback of picking
    if(picking.selected)
    {
        const vec3& p = shape.position[picking.index];    // Current position of the picked vertex

        // Display the selected vertex as a sphere
        sphere_picking.uniform.transform.translation = p;
        draw(sphere_picking, scene.camera);

        // Display falloff distance as a circle oriented along the normal of the shape
        const mat3 R = rotation_between_vector_mat3({0,0,1}, shape.normal[picking.index]);
        falloff_circle.uniform.transform.translation = p;
        falloff_circle.uniform.transform.rotation = R;
        falloff_circle.uniform.transform.scaling = gui_scene.falloff;
        draw(falloff_circle, scene.camera);
    }

}






void scene_model::mouse_move(scene_structure& scene, GLFWwindow* window)
{
    // Cursor coordinates
    const vec2 cursor = glfw_cursor_coordinates_window(window);

    // Check that the mouse is clicked (drag and drop)
    const bool mouse_click_left  = glfw_mouse_pressed_left(window);
    const bool key_shift = glfw_key_shift_pressed(window);

    // Selection (press on shift key without clicking on the mouse)
    if(key_shift && !mouse_click_left)
    {
        // Create the 3D ray passing by the selected point on the screen
        const ray r = picking_ray(scene.camera, cursor);

        // Check if this ray intersects a position (represented by a sphere)
        //  Loop over all positions and get the intersected position (the closest one in case of multiple intersection)
        picking.selected = false;
        float distance_min = 0.0f;
        const int N = shape.position.size();
        for(int k=0; k<N; ++k)
        {
            const vec3& p = shape.position[k];
            const picking_info info = ray_intersect_sphere(r, p, 0.03f);

            if( info.picking_valid ) // the ray intersects a sphere
            {
                const float distance = norm(info.intersection-r.p); // get the closest intersection

                if( picking.selected==false || distance<distance_min ) {
                    assert( int(shape.normal.size())>=k );
                    const vec3& n = shape.normal[k];
                    picking = {true, k, cursor, p, n};  // Store current picking data information
                    distance_min = distance;
                }

            }
        }

    }

    // Deformation (press on shift key + left click on the mouse when a vertex is already selected)
    if(mouse_click_left && key_shift && picking.selected)
    {
        // Current translation in 2D window coordinates
        const vec2 t2D = cursor - picking.s0;

        // Apply the deformation on the surface
        update_deformation(scene.camera, t2D);

        // Update the visual model
        visual.update_position(shape.position);
    }

    // Unselect picking when shift is released
    if(!key_shift)
        picking.selected = false;
}


void scene_model::mouse_click(scene_structure& , GLFWwindow* window, int , int , int )
{

    const bool mouse_release_left  = glfw_mouse_release_left(window);
    if(mouse_release_left && picking.selected){
        // Clear picking and update normals when the mouse button is released

        picking.selected = false;
        position_saved = shape.position;

        normal(shape.position, shape.connectivity, shape.normal);
        visual.update_normal(shape.normal);
    }
}


void scene_model::mouse_scroll(scene_structure& , GLFWwindow* , float , float y_offset)
{
    // Increase/decrease falloff distance when scrolling the mouse
    if(picking.selected)
        gui_scene.falloff = std::max(gui_scene.falloff + y_offset*0.01f, 1e-6f);
}


void scene_model::set_gui()
{

    ImGui::Checkbox("Wireframe", &gui_scene.wireframe); // Display wireframe
    ImGui::Checkbox("Normals", &gui_scene.normals);     // Display normals

    ImGui::Text("Surface type:"); // Select surface to be deformed
    int* ptr_surface_type  = reinterpret_cast<int*>(&gui_scene.surface_type); // trick - use pointer to select enum
    const int new_plane    = ImGui::RadioButton("Plane",ptr_surface_type, surface_plane); ImGui::SameLine();
    const int new_cylinder = ImGui::RadioButton("Cylinder",ptr_surface_type, surface_cylinder); ImGui::SameLine();
    const int new_sphere   = ImGui::RadioButton("Sphere",ptr_surface_type, surface_sphere); ImGui::SameLine();
    const int new_cube     = ImGui::RadioButton("Cube",ptr_surface_type, surface_cube);  ImGui::SameLine();
    const int new_mesh     = ImGui::RadioButton("Mesh",ptr_surface_type, surface_mesh);

    // Call specific initialization if one of the button is clicked
    if(new_plane)         { initialize_plane(); }
    else if(new_cylinder) { initialize_cylinder(); }
    else if(new_sphere)   { initialize_sphere(); }
    else if(new_cube)     { initialize_cube(); }
    else if(new_mesh)     { initialize_mesh(); }


    ImGui::Text("Deformer Type:"); // Select type of deformation
    int* ptr_deformer_type = (int*)&gui_scene.deformer_type;
    ImGui::RadioButton("Translate",ptr_deformer_type, deform_translate); ImGui::SameLine();
    ImGui::RadioButton("Twist",ptr_deformer_type, deform_twist); ImGui::SameLine();
    ImGui::RadioButton("Scale",ptr_deformer_type, deform_scale);


    ImGui::Text("Deformer direction:"); // Select type of deformation direction
    int* ptr_deformer_direction = reinterpret_cast<int*>(&gui_scene.deformer_direction);
    ImGui::RadioButton("Screen",ptr_deformer_direction, camera_direction); ImGui::SameLine();
    ImGui::RadioButton("Normal",ptr_deformer_direction, normal_direction);

    // Select falloff distance using slider
    ImGui::SliderFloat("Falloff distance", &gui_scene.falloff, 0.01f, 0.8f);
}


void scene_model::initialize_plane()
{
    const int N = 150;
    shape = mesh_primitive_grid(N, N, {-1,-1,0}, {2,0,0}, {0,2,0});
    initialize_visual_mesh(shaders["mesh_bf"]);
}

void scene_model::initialize_cylinder()
{
    const float h = 1.5f;
    const float radius = 0.4f;
    const float N_sample_circumferential = 80;
    const float N_sample_length = int( h/(2*3.14f*radius)*(N_sample_circumferential-1) + 1 + 0.5f );
    shape = mesh_primitive_cylinder(radius, {0,-h/2,0}, {0,h/2,0}, N_sample_circumferential, N_sample_length);
    initialize_visual_mesh(shaders["mesh_bf"]);
}

void scene_model::initialize_sphere()
{
    const int N = 100;
    const float radius = 0.75f;
    shape = mesh_primitive_sphere(radius, {0,0,0}, N, 2*N);
    initialize_visual_mesh(shaders["mesh"]);
}
void scene_model::initialize_cube()
{
    const int N = 70;
    shape = mesh_primitive_parallelepiped_grid(N, N, {-0.5,-0.5,-0.5}, {1,0,0}, {0,1,0}, {0,0,1});
    initialize_visual_mesh(shaders["mesh"]);
}

void scene_model::initialize_mesh()
{
    const std::string filename = "scenes/sources/deformation/local_deformers/assets/face.obj";
    shape = mesh_load_file_obj(filename);
    for(auto& p : shape.position) p *= 0.5f;
    shape.fill_empty_fields();
    initialize_visual_mesh(shaders["mesh"]);
}

void scene_model::initialize_visual_mesh(GLuint shader)
{
    position_saved = shape.position;
    normals_saved = shape.normal;

    visual = shape;
    visual.shader = shader;
    visual.uniform.color = {1,1,1};
}

#endif

