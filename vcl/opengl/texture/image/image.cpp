#include "image.hpp"

#include "vcl/math/math.hpp"

namespace vcl
{

//void convert_to_image(buffer2D<vec3> const& value, image_rgb& im)
//{
//    size_t const Nx = value.size()[0];
//    size_t const Ny = value.size()[1];

//    if(im.size()[0]!=Nx || im.size()[1]!=Ny)
//        im.resize(Nx,Ny);

//    size_t const N = Nx*Ny;
//    for(size_t k=0; k<N; ++k){
//        const vec3& v = value[k];

//        unsigned char const r = clamp(255*v.x, 0, 255);
//        unsigned char const g = clamp(255*v.y, 0, 255);
//        unsigned char const b = clamp(255*v.z, 0, 255);

//        im[k] = {r,g,b};
//    }

//}

buffer2D<vec3> image_raw::to_buffer_rgb() const
{
    buffer2D<vec3> b;
    b.resize(width, height);

    for(size_t i=0; i<width; ++i){
        for(size_t j=0; j<height; ++j){
            size_t const k = i+width*j;

            if(color_type==image_color_type::rgb)
                b(i,height-1-j) = {data[3*k]/255.0f, data[3*k+1]/255.0f, data[3*k+2]/255.0f};
            else
                b(i,height-1-j) = {data[4*k]/255.0f, data[4*k+1]/255.0f, data[4*k+2]/255.0f};
        }
    }

    return b;
}

}
