#include "opengl_helper.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace vcl;

void OGLHelper::initializeFBO(GLuint fbo[3], size_t width, size_t height){
  glGenFramebuffers(1, &fbo[0]);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);

  glGenTextures(1, &fbo[1]);
  glBindTexture(GL_TEXTURE_2D, fbo[1]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo[1], 0);

  glGenRenderbuffers(1, &fbo[2]);
  glBindRenderbuffer(GL_RENDERBUFFER, fbo[2]);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo[2]);

  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
     std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
