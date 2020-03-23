#pragma once

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <string>

#include "vcl/vcl.hpp"

struct OGLHelper {

  void initializeFBO(GLuint fbo[3], bool highResolution = false, size_t width = 2560, size_t height = 2000);

  void drawOn(GLuint buffer_id, GLuint shader, bool reverseDepth);

};
