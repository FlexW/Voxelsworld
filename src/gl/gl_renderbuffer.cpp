#include "gl_renderbuffer.hpp"

GlRenderbuffer::GlRenderbuffer() { glGenRenderbuffers(1, &id_); }

GlRenderbuffer::~GlRenderbuffer() { glDeleteRenderbuffers(1, &id_); }

void GlRenderbuffer::set_storage(GLenum  internal_format,
                                 GLsizei width,
                                 GLsizei height)
{
  glBindRenderbuffer(GL_RENDERBUFFER, id_);
  glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void GlRenderbuffer::set_storage_multisample(GLenum  internal_format,
                                             GLsizei samples,
                                             GLsizei width,
                                             GLsizei height)
{
  glBindRenderbuffer(GL_RENDERBUFFER, id_);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                                   samples,
                                   internal_format,
                                   width,
                                   height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

GLuint GlRenderbuffer::id() const { return id_; }
