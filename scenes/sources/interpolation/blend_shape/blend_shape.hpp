#pragma once

#include "scenes/base/base.hpp"


#ifdef SCENE_BLEND_SHAPE


struct gui_scene_structure
{
    bool wireframe   = false; // Display the wireframe
    int face_index = 0;

};



struct scene_model : scene_base
{

    void setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);
    void frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui);

    void set_gui();
    void update_face();

    gui_scene_structure gui_scene;

    std::vector<vcl::mesh> faces;
    vcl::mesh_drawable visual;
    vcl::mesh_drawable body;


};

#endif


