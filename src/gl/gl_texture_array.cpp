#include "gl_texture_array.hpp"

#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/integer.hpp>

#include <cassert>

GlTextureArray::GlTextureArray() { glGenTextures(1, &id_); }

GlTextureArray::~GlTextureArray() { glDeleteTextures(1, &id_); }

GLuint GlTextureArray::id() const { return id_; }

void GlTextureArray::set_data(
    const std::vector<SubTextureData> &sub_textures_data)
{
  assert(sub_textures_data.size() > 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, id_);

  const auto width  = sub_textures_data[0].width;
  const auto height = sub_textures_data[0].height;

  glTexStorage3D(GL_TEXTURE_2D_ARRAY,
                 glm::log2(glm::max(width, height)) + 1,
                 GL_RGBA8,
                 sub_textures_data[0].width,
                 sub_textures_data[0].height,
                 sub_textures_data.size());

  glTexParameteri(GL_TEXTURE_2D_ARRAY,
                  GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  for (std::size_t i = 0; i < sub_textures_data.size(); ++i)
  {
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                    0,
                    0,
                    0,
                    i,
                    sub_textures_data[i].width,
                    sub_textures_data[i].height,
                    1,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    sub_textures_data[i].data);
  }

  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
