
#include "laplacian_editing.hpp"
#ifdef SCENE_LAPLACIAN_EDITING

#include "Eigen/SVD"
#include <set>



using namespace vcl;

// Offset withing the 2D grid
static int offset(int ku, int kv, int Nv);

void scene_model::initialize_shape()
{
    const int N = 15;
    shape = mesh_primitive_grid(N,N, {-1,-1,0}, {2,0,0}, {0,2,0});
    constraints.fixed[offset(0,0,N)]      = shape.position[offset(0,0,N)];
    constraints.fixed[offset(N-1,0,N)]    = shape.position[offset(N-1,0,N)];
    constraints.fixed[offset(0,N-1,N)]    = shape.position[offset(0,N-1,N)];
    constraints.fixed[offset(N-1,N-1,N)]  = shape.position[offset(N-1,N-1,N)];
    constraints.target[offset(N/2,N/2,N)] = shape.position[offset(N/2,N/2,N)];


    visual.clear();
    visual = shape;
    visual.shader = shader_mesh;
    visual.uniform.color = {1,1,1};


}



void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& , gui_structure& gui)
{
    shader_mesh = shaders["mesh_bf"];
    initialize_shape();

    // Initialise sphere used to display selected and constrained vertices
    sphere = mesh_primitive_sphere(0.02f);
    sphere.shader = shaders["mesh"];

    build_matrix();
    update_deformation();

    timer.periodic_event_time_step = 0.15f;
    picking.segment_drawer.init();

    gui.show_frame_camera = false;
}


void scene_model::build_matrix()
{
    // Fill the matrix structure here ...

}

void scene_model::update_deformation()
{

    // Solve the linear system here and update the shape...


    need_update = false;
}







void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    set_gui();

    // Draw shapes
    draw(visual, scene.camera);
    if( gui_scene.wireframe )
        draw(visual, scene.camera, shaders["wireframe"]);

    display_constraints(scene);

    if(constraints_selection_mode)
        diplay_selector_rectangle(scene, shaders); // Display the current selector (rectangle)

    // Update the shape if needed
    if(need_update) {
        timer.update();
        if( timer.event )
            update_deformation();
    }
}


int offset(int ku, int kv, int Nv)
{
    return kv+Nv*ku;
}


void scene_model::display_constraints(scene_structure const& scene)
{
    // Display fixed positional constraints
    sphere.uniform.transform.scaling = 1.0f;
    sphere.uniform.color = {0,0,1};
    for(const auto& c : constraints.fixed) {
        const int k = c.first;
        sphere.uniform.transform.translation = shape.position[k];
        draw(sphere, scene.camera);
    }

    // Display temporary constraints in yellow
    sphere.uniform.color = {1,1,0};
    for(int idx : picking.constraints_temporary) {
        sphere.uniform.transform.translation = shape.position[idx];
        draw(sphere, scene.camera);
    }


    // Display target constraints
    for(const auto& c : constraints.target) {
        const int k = c.first;
        const vec3& p = c.second;

        // The real target position in white
        sphere.uniform.color = {1,1,1};
        sphere.uniform.transform.scaling = 0.9f;
        sphere.uniform.transform.translation = p;
        draw(sphere, scene.camera);

        // The achieved position in red
        sphere.uniform.color = {1,0,0};
        sphere.uniform.transform.scaling = 1.0f;
        sphere.uniform.transform.translation = shape.position[k];
        draw(sphere, scene.camera);
    }
}
void scene_model::diplay_selector_rectangle(scene_structure const& scene, std::map<std::string,GLuint> const& shaders)
{
    // Get 3D position of the four corners selected on screen
    ray r0 = picking_ray(scene.camera, picking.selection_p0);
    ray r1 = picking_ray(scene.camera, {picking.selection_p0.x,picking.selection_p1.y});
    ray r2 = picking_ray(scene.camera, picking.selection_p1);
    ray r3 = picking_ray(scene.camera, {picking.selection_p1.x, picking.selection_p0.y});

    vec3 p0 = r0.p + r0.u;
    vec3 p1 = r1.p + r1.u;
    vec3 p2 = r2.p + r2.u;
    vec3 p3 = r3.p + r3.u;


    // Display the four segments
    picking.segment_drawer.uniform_parameter.p1 = p0;
    picking.segment_drawer.uniform_parameter.p2 = p1;
    picking.segment_drawer.draw(shaders.at("segment_im"), scene.camera);
    picking.segment_drawer.uniform_parameter.p1 = p1;
    picking.segment_drawer.uniform_parameter.p2 = p2;
    picking.segment_drawer.draw(shaders.at("segment_im"), scene.camera);
    picking.segment_drawer.uniform_parameter.p1 = p2;
    picking.segment_drawer.uniform_parameter.p2 = p3;
    picking.segment_drawer.draw(shaders.at("segment_im"), scene.camera);
    picking.segment_drawer.uniform_parameter.p1 = p3;
    picking.segment_drawer.uniform_parameter.p2 = p0;
    picking.segment_drawer.draw(shaders.at("segment_im"), scene.camera);
}



void scene_model::keyboard_input(scene_structure& , GLFWwindow* , int key, int , int action, int )
{
    // Key S = Shortcut to switch between constraint selection/displacement
    if(key==GLFW_KEY_S && action==GLFW_PRESS) {
        constraints_selection_mode = !constraints_selection_mode;
    }
}

void scene_model::mouse_click(scene_structure& , GLFWwindow* window, int button, int action, int )
{
    bool const mouse_released_left   = (button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_RELEASE);
    bool const mouse_released_right  = (button==GLFW_MOUSE_BUTTON_RIGHT && action==GLFW_RELEASE);
    bool const key_shift = glfw_key_shift_pressed(window);

    const vec2 cursor = glfw_cursor_coordinates_window(window);

    // Reset selection at every click pressed/released
    picking.selection_p0 = cursor;
    picking.selection_p1 = cursor;

    // Click in selection mode
    if(constraints_selection_mode && key_shift) {

        // Release left click add target constraints (convert constraints_temporary to target)
        if(mouse_released_left) {
            if(picking.constraints_temporary.empty()) // Release the mouse with no position => clear all target constraints
                constraints.target.clear();
            else{
                // Otherwise add new target constraints
                for(int idx : picking.constraints_temporary) {
                    constraints.target[idx] = shape.position[idx];

                    if( constraints.fixed.find(idx)!=constraints.fixed.end() )
                        constraints.fixed.erase(idx); // Erase fixed constraints if it is now a target constraint
                }
            }
        }

        // Release right click add fixed constraints (convert constraints_temporary to fixed)
        if(mouse_released_right) {
            if(picking.constraints_temporary.empty()) // Release the mouse with no position => clear all fixed constraints
                constraints.fixed.clear();
            else{
                // Otherwise add new fixed constraints
                for(int idx : picking.constraints_temporary) {
                    constraints.fixed[idx] = shape.position[idx];
                    if( constraints.target.find(idx)!=constraints.target.end() )
                        constraints.target.erase(idx); // Erase target constraints if it is now a fixed constraint
                }
            }
        }
    }


    // If mouse click/released in selection mode: rebuild the matrix
    if(key_shift && constraints_selection_mode){
        build_matrix();
        update_deformation();
        picking.constraints_temporary.clear();
    }

}

void scene_model::mouse_move(scene_structure& scene, GLFWwindow* window)
{
    const vec2 cursor = glfw_cursor_coordinates_window(window);

    bool const mouse_click_left  = glfw_mouse_pressed_left(window);
    bool const mouse_click_right  = glfw_mouse_pressed_right(window);
    bool const key_shift = glfw_key_shift_pressed(window);

    // Select new constraints using rectangle on screen
    if(constraints_selection_mode){
        if( (mouse_click_left || mouse_click_right) && key_shift) {
            picking.selection_p1 = cursor;
            picking.constraints_temporary.clear();

            // Compute extremal coordinates of the selection box
            float const x_min = std::min(picking.selection_p0.x, picking.selection_p1.x);
            float const x_max = std::max(picking.selection_p0.x, picking.selection_p1.x);
            float const y_min = std::min(picking.selection_p0.y, picking.selection_p1.y);
            float const y_max = std::max(picking.selection_p0.y, picking.selection_p1.y);

            mat4 const modelview = scene.camera.perspective.matrix() * scene.camera.view_matrix();
            size_t const N = shape.position.size();
            for(size_t k=0; k<N; ++k)
            {
                // Compute projection coordinate of each vertex
                vec3 const& p = shape.position[k];
                vec4 p_screen = modelview*vec4(p.x,p.y,p.z,1.0f);
                p_screen /= p_screen.w;

                // Check if the projected coordinates are within the screen box
                float const x = p_screen.x;
                float const y = p_screen.y;
                if(x>x_min && x<x_max && y>y_min && y<y_max)
                    picking.constraints_temporary.insert(k); // add a new possible constraint in the selection
            }

        }
    }
    // Otherwise can displace constraints using shift + drag and drop
    else
    {
        if( (mouse_click_left || mouse_click_right) && key_shift) {
            picking.selection_p1 = cursor;

            vec2 const tr_2D = cursor-picking.cursor_previous; // translation in screen coordinates
            vec3 const tr = scene.camera.orientation * vec3(tr_2D.x, tr_2D.y, 0.0f); // translation in 3D

            // Apply the translation to all target constraints
            for(auto& it : constraints.target) {
                vec3& p = it.second;
                p += tr;
            }

            need_update = true; // The surface can be updated
        }
    }

    picking.cursor_previous = cursor;
}




void scene_model::set_gui()
{

    ImGui::Checkbox("Wireframe", &gui_scene.wireframe);

    bool change_active_weight  = ImGui::SliderFloat("Weight Fixed", &constraints.weight_fixed, 0.05f, 10.0f, "%.3f", 3);
    bool change_passive_weight = ImGui::SliderFloat("Weight Target", &constraints.weight_target, 0.05f, 10.0f, "%.3f", 3);
    if(change_active_weight || change_passive_weight) {
        build_matrix();
        update_deformation();
    }


    ImGui::Checkbox("Select constraint", &constraints_selection_mode);
}



#endif

