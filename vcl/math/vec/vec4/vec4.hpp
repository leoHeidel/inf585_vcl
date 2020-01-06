#pragma once

#include "vcl/containers/buffer_stack/buffer_stack.hpp"

namespace vcl {

/** vec4 is an alias on the generic buffer_stack<float, 4> */
using vec4 = buffer_stack<float, 4>;

template <> struct buffer_stack<float, 4> {

    float x;
    float y;
    float z;
    float w;

    buffer_stack<float, 4>();
    buffer_stack<float, 4>(float x,float y,float z,float w);

    const float& operator[](std::size_t index) const;
    float& operator[](std::size_t index);

};



}
