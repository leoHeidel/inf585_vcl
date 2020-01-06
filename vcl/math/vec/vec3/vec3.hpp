#pragma once

#include "vcl/containers/buffer_stack/buffer_stack.hpp"

namespace vcl {

/** vec3 is an alias on a generic buffer_stack<float, 3> */
using vec3 = buffer_stack<float, 3>;

/** vec3 is a specialized-template class from a generic vec<N> */
template <> struct buffer_stack<float, 3> {

    /** Three x,y,z coordinates of the vector */
    float x;
    float y;
    float z;

	buffer_stack<float, 3>();
	buffer_stack<float, 3>(float x,float y,float z);

    /** Get operator at given index */
    const float& operator[](std::size_t index) const;
    /** Set operator at given index */
    float& operator[](std::size_t index);
};

/** Cross product between two vec3 */
vec3 cross(const vec3& a,const vec3& b);


}
