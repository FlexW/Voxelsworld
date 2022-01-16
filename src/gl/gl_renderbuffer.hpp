#pragma once

#include "gl.hpp"

class GlRenderbuffer
{
public:
  GlRenderbuffer();
  ~GlRenderbuffer();

  void set_storage(GLenum internal_format, GLsizei width, GLsizei height);
  void set_storage_multisample(GLenum  internal_format,
                               GLsizei samples,
                               GLsizei width,
                               GLsizei height);

  GLuint id() const;

private:
  GLuint id_{};

  GlRenderbuffer(const GlRenderbuffer &other) = delete;
  void operator=(const GlRenderbuffer &other) = delete;
  GlRenderbuffer(GlRenderbuffer &&other)      = delete;
  void operator=(GlRenderbuffer &&other) = delete;
};
