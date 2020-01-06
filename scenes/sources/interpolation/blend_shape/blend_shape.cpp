
#include "blend_shape.hpp"


#ifdef SCENE_BLEND_SHAPE


using namespace vcl;





void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    // Load faces
    faces.push_back(mesh_load_file_obj("scenes/sources/interpolation/blend_shape/assets/face_00.obj"));
    faces.push_back(mesh_load_file_obj("scenes/sources/interpolation/blend_shape/assets/face_01.obj"));
    faces.push_back(mesh_load_file_obj("scenes/sources/interpolation/blend_shape/assets/face_02.obj"));
    faces.push_back(mesh_load_file_obj("scenes/sources/interpolation/blend_shape/assets/face_03.obj"));
    faces.push_back(mesh_load_file_obj("scenes/sources/interpolation/blend_shape/assets/face_04.obj"));
    faces.push_back(mesh_load_file_obj("scenes/sources/interpolation/blend_shape/assets/face_05.obj"));


    for(size_t k=0; k<faces.size(); ++k)
        faces[k].fill_empty_fields(); // Make sure normals are precomputed

    // Visual model of the face
    visual = faces[0];
    visual.shader = shaders["mesh"];

    // Load the corresponding body
    body = mesh_load_file_obj("scenes/sources/interpolation/blend_shape/assets/body.obj");
    body.shader = shaders["mesh"];



    // Initial position of the camera
    scene.camera.translation = {0.0f, -6.1f, 0.0f};
    scene.camera.scale = 5.0f;
    gui.show_frame_camera = false;



}





void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    set_gui();

    draw(visual, scene.camera);
    draw(body, scene.camera);

    if(gui_scene.wireframe == true) {
        draw(visual, scene.camera, shaders["wireframe"]);
        draw(body, scene.camera, shaders["wireframe"]);
    }

}

// Display the selected face
void scene_model::update_face()
{
    mesh const& current = faces[gui_scene.face_index];
    visual.update_position(current.position);
    visual.update_normal(current.normal);
}



void scene_model::set_gui()
{
    ImGui::Checkbox("Wireframe", &gui_scene.wireframe);

    bool const change_face = ImGui::SliderInt("Face index", &gui_scene.face_index, 0, faces.size()-1);
    if( change_face )
        update_face();


}



#endif

