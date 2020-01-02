#pragma once

#include <vector>

#include "vcl/containers/buffer/buffer2D/buffer2D.hpp"
#include "vcl/math/vec/vec3/vec3.hpp"

namespace vcl
{

//using rgb  = std::array<unsigned char,3>;
//using rgba = std::array<unsigned char,4>;

//using image_rgb  = buffer2D<rgb>;
//using image_rgba = buffer2D<rgba>;

enum class image_color_type {rgb, rgba};

struct image_raw
{
    unsigned int width;
    unsigned int height;
    image_color_type color_type;
    std::vector<unsigned char> data;

//    unsigned char const& operator()(size_t kx, size_t ky) const { return data[ky+height*kx]; }
//    unsigned char& operator()(size_t kx, size_t ky) { return data[ky+height*kx]; }
    //void resize(size_t new_width, size_t new_height) {data.resize(4*new_width*new_height); width=new_width; height=new_height;}
    buffer2D<vec3> to_buffer_rgb() const;
};






//void convert_to_image(buffer2D<vec3> const& value, image_rgb& im);

}
