#pragma once

#include "vcl/containers/buffer_stack/buffer_stack.hpp"

namespace vcl {

/** vec2 is an alias on a generic buffer_stack<float, 2> */
using vec2 = buffer_stack<float, 2>;

template <> struct buffer_stack<float, 2> {

    float x;
    float y;

    buffer_stack<float, 2>();
    buffer_stack<float, 2>(float x,float y);

    const float& operator[](std::size_t index) const;
    float& operator[](std::size_t index);
};



}
