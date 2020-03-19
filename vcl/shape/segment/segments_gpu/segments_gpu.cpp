#include "segments_gpu.hpp"

#include "vcl/base/base.hpp"
#include "vcl/opengl/debug/opengl_debug.hpp"

namespace vcl
{

segments_gpu::segments_gpu()
    :vao(0),vbo_position(0),number_elements(0)
{}

segments_gpu::segments_gpu(const std::vector<vec3>& position)
    :vao(0),vbo_position(0),number_elements(0)
{
    // Fill VBO for position
    glGenBuffers(1, &vbo_position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(position.size()*sizeof(GLfloat)*3), &position[0], GL_DYNAMIC_DRAW );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    number_elements = static_cast<unsigned int>(position.size());

    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);

    // position at layout 0
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}


void draw(const segments_gpu& curve)
{
    glBindVertexArray(curve.vao); opengl_debug();
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);opengl_debug();
    glDrawArrays(GL_POINTS, 0, GLsizei(curve.number_elements) ); opengl_debug();
    glBindVertexArray(0);
}

void drawSpheres(const segments_gpu& curve)
{
    glBindVertexArray(curve.vao); opengl_debug();
    //GLuint texture_id = create_texture_gpu(image_load_png("scenes/sources/incompressible_sph/textures/gary.png"), GL_REPEAT, GL_REPEAT );
    //glBindTexture(GL_TEXTURE_2D, texture_id);opengl_debug();
    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_BLEND); opengl_debug();
    //glEnable(GL_POINT_SPRITE); opengl_debug();
    glColor4f(0, 0, 1, 1);
    glEnable(GL_POINT_SMOOTH);
    glPointSize (32.0);
    glDrawArrays(GL_POINTS, 0, GLsizei(curve.number_elements) ); opengl_debug();
    //glTexEnvi(0x8861, 0x8862, GL_FALSE);opengl_debug();
    //glBindTexture(GL_TEXTURE_2D, 0);opengl_debug();
    //glDisable(GL_TEXTURE_2D);opengl_debug();
    glBindVertexArray(0);
}

}
