#pragma once

#include <glad/glad.h>

#include <vector>

class GlTextureArray
{
public:
  struct SubTextureData
  {
    SubTextureData(unsigned char *data, int width, int height)
        : data(data),
          width(width),
          height(height)
    {
    }

    unsigned char *data;
    int            width;
    int            height;
  };

  GlTextureArray();
  ~GlTextureArray();

  GLuint id() const;

  void set_data(const std::vector<SubTextureData> &sub_textures_data);

private:
  GLuint id_{};

  GlTextureArray(const GlTextureArray &) = delete;
  void operator=(const GlTextureArray &) = delete;
  GlTextureArray(GlTextureArray &&)      = delete;
  void operator=(GlTextureArray &&) = delete;
};
