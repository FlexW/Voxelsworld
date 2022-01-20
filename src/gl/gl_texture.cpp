#include "gl_texture.hpp"
#include "image.hpp"

#include <cassert>
#include <stdexcept>

GlTexture::GlTexture() { glGenTextures(1, &texture_id_); }

GlTexture::~GlTexture() { glDeleteTextures(1, &texture_id_); }

GLuint GlTexture::id() const { return texture_id_; }

void GlTexture::load_from_file(const std::filesystem::path &file_path,
                               bool                         generate_mipmap)
{
  const Image image{file_path};
  set_data(image.data(),
           image.width(),
           image.height(),
           image.channels_count(),
           generate_mipmap);
}

void GlTexture::set_data(unsigned char *data,
                         int            width,
                         int            height,
                         int            channels_count,
                         bool           generate_mipmap)
{
  bind();

  // Figure out the image format
  GLenum format{};
  GLint  internal_format{};
  if (channels_count == 1)
  {
    format          = GL_RED;
    internal_format = GL_RED;
  }
  else if (channels_count == 3)
  {
    format          = GL_RGB;
    internal_format = GL_RGB;
  }
  else if (channels_count == 4)
  {
    format          = GL_RGBA;
    internal_format = GL_RGBA;
  }
  else
  {
    throw std::runtime_error("Can not handle channel count");
  }

  // Send the image data to the GPU
  glTexImage2D(GL_TEXTURE_2D,
               0,
               internal_format,
               width,
               height,
               0,
               format,
               GL_UNSIGNED_BYTE,
               data);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if (generate_mipmap)
  {
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  // Generate mipmaps if needed
  if (generate_mipmap)
  {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  unbind();
}

void GlTexture::bind() { glBindTexture(GL_TEXTURE_2D, texture_id_); }

void GlTexture::unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

void GlTexture::set_storage(GLsizei width,
                            GLsizei height,
                            GLint   internal_format,
                            GLenum  format)
{
  bind();

  glTexImage2D(GL_TEXTURE_2D,
               0,
               internal_format,
               width,
               height,
               0,
               format,
               GL_UNSIGNED_BYTE,
               nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  unbind();
}
