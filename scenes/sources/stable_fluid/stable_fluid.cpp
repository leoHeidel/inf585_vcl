#include "stable_fluid.hpp"


#ifdef SCENE_STABLE_FLUID

using namespace vcl;

void set_boundary(velocity_grid& grid);
template <typename T> void set_boundary(buffer2D<T>& grid);

template <typename T>
void diffuse(buffer2D<T>& f, buffer2D<T> const& f_prev, float mu, float dt)
{
    // Compute diffusion on f
    //  Use f as current value, f_prev as previous value
    //  The function is generic in order to handle f as being either a velocity (T=vec2), or a color density (T=vec3)



    const size_t Nx = f.dimension[0];
    const size_t Ny = f.dimension[1];

    // Fill this function
    // Gauss Seidel iterations
    //    for all (x,y), f(x,y) = ...
    //    set_boundary(f)


}



template <typename T>
void advect(buffer2D<T>& value, buffer2D<T> const& value_prev, velocity_grid const& v, float dt)
{
    // Compute advection of value along the velocity v, given its previous state value_prev

    const size_t Nx = v.dimension[0];
    const size_t Ny = v.dimension[1];


    for(size_t x=1; x<Nx-1; ++x) {
        for(size_t y=1; y<Ny-1; ++y) {

            // Fill the correct advected value
            // value(x,y) = ...

        }
    }

}


void project(velocity_grid& v, velocity_grid const& v0, scalar_grid& divergence, scalar_grid& gradient_field)
{
    // v = projection of v0 on divergence free vector field
    //
    // v : Final vector field to be filled
    // v0: Initial vector field (non divergence free)
    // divergence: temporary buffer used to compute the divergence of v0
    // gradient_field: temporary buffer used to compute v = v0 - nabla(gradient_field)


    // 1. Compute divergence of v0
    // 2. Compute gradient_field such that nabla(gradient_field)^2 = div(v0)
    // 3. Compute v = v0 - nabla(gradient_field)

}


void scene_model::fluid_evolve(float dt)
{

    diffuse(velocity, velocity_prev, diffuse_coefficient_velocity, dt); velocity_prev = velocity;
    project(velocity, velocity_prev, divergence, gradient_field); velocity_prev = velocity;
    advect(velocity, velocity_prev, velocity_prev, dt); velocity_prev = velocity;


    diffuse(density, density_save, diffuse_coefficient_density, dt); density_save = density;
    advect(density, density_save, velocity, dt);
}




void scene_model::initialize_fields(density_type_structure density_type)
{
    int const Nx = 60;
    int const Ny = 60;

    // Initialize velocity
    velocity.resize(Nx,Ny);
    velocity.fill({0,0});
    velocity_prev = velocity;

    if(density_type == density_color) {
        // Initialize density
        density.resize(Nx,Ny);
        density.fill({1,1,1});
        for(int kx=Nx/6; kx<Nx/2; ++kx)
            for(int ky=Ny/6; ky<5*Ny/6; ++ky)
                density(kx,ky) = {1,0,0};
        for(int kx=Nx/2; kx<5*Nx/6; ++kx)
            for(int ky=Ny/6; ky<5*Ny/6; ++ky)
                density(kx,ky) = {0,1,0};
    }

    if(density_type == density_texture)
        density = image_load_png("scenes/sources/stable_fluid/assets/texture.png").to_buffer_rgb();


    divergence = scalar_grid(Nx,Ny);
    gradient_field = scalar_grid(Nx,Ny);
}

void scene_model::setup_data(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& gui)
{
    initialize_fields(density_color);

    int const Nx = velocity.dimension[0];
    int const Ny = velocity.dimension[1];

    setup_visual_grid(Nx, Ny, shaders["curve"]);

    //density_image.resize(Nx,Ny);
    texture_id = create_texture_gpu(density);

    float const dx = 1.0f/(Nx-1.0f);
    float const dy = 1.0f/(Ny-1.0f);
    quad = mesh_primitive_quad({-dx/2,-dy/2,0},{1+dx/2,-dy/2,0},{1+dx/2,1+dy/2,0},{-dx/2,1+dy/2,0});
    quad.uniform.shading = {1.0f, 0.0f, 0.0f, 1};
    quad.shader = shaders["mesh"];
    quad.texture_id = texture_id;

    segment_drawer.init();
    scene.camera.scale = 1.7f;
    gui.show_frame_camera = false;
}

template <typename T>
void set_boundary_corners(buffer2D<T>& grid)
{
    size_t const Nx = grid.dimension[0];
    size_t const Ny = grid.dimension[1];

    grid(0,0)       = (grid(1,0) + grid(0,1))/2.0f;
    grid(Nx-1,0)    = (grid(Nx-2,0) + grid(Nx-1,1))/2.0f;
    grid(0,Ny-1)    = (grid(1,Ny-1) + grid(0,Ny-2))/2.0f;
    grid(Nx-1,Ny-1) = (grid(Nx-2,Ny-1) + grid(Nx-1,Ny-2))/2.0f;
}

template <typename T>
void set_boundary(buffer2D<T>& grid)
{
    size_t const Nx = grid.dimension[0];
    size_t const Ny = grid.dimension[1];

    for(size_t x=1; x<Nx-1; ++x) {
        grid(x,0)    = grid(x,1);
        grid(x,Ny-1) = grid(x,Ny-2);
    }

    for(size_t y=1; y<Ny-1; ++y) {
        grid(0,y)    = grid(1,y);
        grid(Nx-1,y) = grid(Nx-2,y);
    }

    set_boundary_corners(grid);
}

// Set boundary for speed
void set_boundary(velocity_grid& grid)
{
    size_t const Nx = grid.dimension[0];
    size_t const Ny = grid.dimension[1];

    for(size_t x=1; x<Nx-1; ++x) {
        grid(x,0)    = grid(x,1)*vec2(1,-1);
        grid(x,Ny-1) = grid(x,Ny-2)*vec2(1,-1);
    }

    for(size_t y=1; y<Ny-1; ++y) {
        grid(0,y)    = grid(1,y)*vec2(-1,1);
        grid(Nx-1,y) = grid(Nx-2,y)*vec2(-1,1);
    }

    set_boundary_corners(grid);
}



void scene_model::frame_draw(std::map<std::string,GLuint>& shaders, scene_structure& scene, gui_structure& )
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    set_gui();

    scene.camera.orientation = mat3::identity();
    scene.camera.translation = {-0.5,-0.5,0};

    float const elapsed = timer.update();
    float const dt = 1*timer.scale;

    // Evolve velocity and density of the fluid
    if(elapsed>0) {
        velocity_prev = velocity;
        density_save = density;
        fluid_evolve(dt);
        velocity_prev = velocity;
        density_save = density;
    }

    // Update density texture
    update_texture_gpu(texture_id, density);

    // Display density
    glPolygonOffset( 1.0, 1.0 );
    if(gui_scene.display_density)
        draw(quad, scene.camera);

    // Display grid
    if(gui_scene.display_grid){
        glPolygonOffset( 1.0, 1.0 );
        draw(visual_grid, scene.camera);
    }

    // Display velocity as vectors
    if(gui_scene.display_velocity)
        display_velocity(shaders["segment_im"], scene);

}


void scene_model::setup_visual_grid(int Nx, int Ny, GLuint shader)
{
    float const e = 1e-4f;
    std::vector<vec3> edges;
    float const dx = 1.0f/(Nx-1.0f);
    float const dy = 1.0f/(Ny-1.0f);
    for(int kx=0; kx<=Nx; ++kx) {
        float const x  = kx*dx;
        edges.push_back( vec3(x-dx/2, -dy/2, e) );
        edges.push_back( vec3(x-dx/2, 1+dy/2, e) );
    }
    for(int ky=0; ky<=Ny; ++ky) {
        float const y  = ky*dy;
        edges.push_back( vec3(-dx/2, y-dy/2, e) );
        edges.push_back( vec3(1+dx/2, y-dy/2, e) );
    }

    visual_grid = edges;
    visual_grid.shader = shader;
    visual_grid.uniform.color = {0,0,0};
}

void scene_model::display_velocity(GLuint shader, scene_structure& scene)
{
    const int Nx = velocity.dimension[0];
    const int Ny = velocity.dimension[1];
    for(int kx=0; kx<Nx; kx++)
    {
        for(int ky=0; ky<Ny; ky++)
        {
            const float x = kx/(Nx-1.0f);
            const float y = ky/(Ny-1.0f);

            segment_drawer.uniform_parameter.color = {0,0,1};
            const float alpha = 0.2f;
            segment_drawer.uniform_parameter.p1 = {x, y, 0.01f};
            segment_drawer.uniform_parameter.p2 = {x+alpha*velocity(kx,ky).x, y+alpha*velocity(kx,ky).y, 0.01f};
            segment_drawer.draw(shader, scene.camera);
        }
    }
}


void scene_model::mouse_move(scene_structure& scene, GLFWwindow* window)
{
    const vec2 cursor = glfw_cursor_coordinates_window(window);
    screen_motion.add(cursor, timer.t);

    bool const mouse_pressed = glfw_mouse_pressed_left(window);
    if(mouse_pressed)
    {
        const size_t Nx = velocity.dimension[0];
        const size_t Ny = velocity.dimension[1];
        const float Lx = 1/(Nx-1.0f);
        const float Ly = 1/(Ny-1.0f);

        // Extremal points of the quadrangle
        vec4 const p0(-Lx/2,-Ly/2,0,1);
        vec4 const p1(1+Lx/2,1+Ly/2,0,1);
        // Project these points in screen space
        mat4 const P = scene.camera.perspective.matrix()*scene.camera.view_matrix();
        vec4 pp0 = P*p0;
        vec4 pp1 = P*p1;

        // normalized coordinates
        pp0 = pp0/pp0.w;
        pp1 = pp1/pp1.w;

        // get index corresponding to mouse position
        int x = int((cursor.x-pp0.x)/(pp1.x-pp0.x)*Nx);
        int y = int((cursor.y-pp0.y)/(pp1.y-pp0.y)*Ny);

        if(x>1 && y>1 && x<int(Nx-1) && y<int(Ny-1)) {
            // Set mouse speed to the corresponding entry of the velocity
            vec2 const v_mouse = screen_motion.speed_avg();
            velocity(x,y) = 5.0f*v_mouse;
        }
    }

}


void scene_model::set_gui()
{

    ImGui::SliderFloat("Timer scale", &timer.scale, 0.1f, 3.0f);
    ImGui::Checkbox("Grid", &gui_scene.display_grid); ImGui::SameLine();
    ImGui::Checkbox("Velocity", &gui_scene.display_velocity); ImGui::SameLine();
    ImGui::Checkbox("Density", &gui_scene.display_density);

    ImGui::SliderFloat("Diffuse coeff. velocity", &diffuse_coefficient_velocity, 0.0f, 0.5f);
    ImGui::SliderFloat("Diffuse coeff. density", &diffuse_coefficient_density, 0.0f, 0.2f);

    ImGui::Text("Restart");
    bool const restart_color = ImGui::Button("Color");
    bool const restart_texture = ImGui::Button("Texture");
    if(restart_color) initialize_fields(density_color);
    if(restart_texture) initialize_fields(density_texture);

}


#endif



