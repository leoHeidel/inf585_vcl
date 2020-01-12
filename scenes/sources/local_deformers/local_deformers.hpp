#pragma once

#include "scenes/base/base.hpp"

#include <map>

#ifdef SCENE_LOCAL_DEFORMER

// Different surfaces to deform
enum surface_type_enum {
    surface_plane,
    surface_cylinder,
    surface_sphere,
    surface_cube,
    surface_mesh
};

// Different type of deformation
enum deformer_type_enum {
    deform_translate, /* Push/pull locally the surface */
    deform_twist,     /* Twist the surface around the selected point */
    deform_scale      /* Contract/extend surface around the selected point */
};

// Orientation of the deformation
enum deformer_direction_enum {
    camera_direction /* Depends on the camera orientation */,
    normal_direction /* Depends on the surface normal */
};


// Possibilities handled by the GUI
struct gui_scene_structure
{
    bool wireframe         = false;   // Display wireframe
    bool normals           = false;   // Display normals
    float falloff          = 1/5.0f;  // Falloff distance (can be adjusted from the GUI or with the mouse scroll)
    deformer_type_enum deformer_type           = deform_translate; // Type of deformation type
    deformer_direction_enum deformer_direction = camera_direction; // Type of deformation direction
    surface_type_enum surface_type             = surface_plane;    // Type of surface to be deformed
};


// Information relative to picking when a vertex is selected
struct picking_data {
    bool selected;         // true when a vertex is selected, false otherwise
    int index;             // index of picked vertex
    vcl::vec2 s0;          // original picked position in screen coordinates
    vcl::vec3 p0;          // original picked position in 3D
    vcl::vec3 n0;          // original normal corresponding to selected position on surface
};



struct scene_model : scene_base
{
    // Function called when a deformation should be applied
    void update_deformation(const vcl::camera_scene& camera, const vcl::vec2& t);


    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    // Called every time the mouse is clicked
    void mouse_click(scene_structure& scene, GLFWwindow* window, int button, int action, int mods);
    // Called every time the mouse moves
    void mouse_move(scene_structure& scene, GLFWwindow* window);
    // Called every time the mouse is scrolled
    void mouse_scroll(scene_structure& scene, GLFWwindow* window, float x_offset, float y_offset);

    void set_gui();

    // Initialize the different type of surface
    void initialize_plane();
    void initialize_cylinder();
    void initialize_sphere();
    void initialize_cube();
    void initialize_mesh();
    void initialize_visual_mesh(GLuint shader);



    gui_scene_structure gui_scene;


    // Mesh of the deformed shape
    vcl::mesh shape;
    // Storage for original position and normals during interactive deformation
    vcl::buffer<vcl::vec3> position_saved;
    vcl::buffer<vcl::vec3> normals_saved;

    // Radius of influence of deformation
    vcl::curve_drawable falloff_circle;

    // Drawable shape
    vcl::mesh_drawable visual;
    vcl::mesh_drawable sphere_picking;

    // Data related to picking
    picking_data picking;

    // Timer used to update the normals regularily during deformation
    vcl::timer_event timer;
    bool require_normal_update = false;

    // Stored shaders
    std::map<std::string, GLuint> shaders;
};

#endif


