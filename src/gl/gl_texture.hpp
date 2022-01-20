#pragma once

#include <glad/glad.h>

#include <filesystem>

class GlTexture
{
public:
  GlTexture();
  ~GlTexture();

  GLuint id() const;

  void load_from_file(const std::filesystem::path &file_path,
                      bool                         generate_mipmap);
  void set_data(unsigned char *data,
                int            width,
                int            height,
                int            channels_count,
                bool           generate_mipmap);
  void set_storage(GLsizei width,
                   GLsizei height,
                   GLint   internal_format,
                   GLenum  format);

  void bind();
  void unbind();

private:
  GLuint texture_id_{};

  GlTexture(const GlTexture &) = delete;
  void operator=(const GlTexture &) = delete;
  GlTexture(GlTexture &&)           = delete;
  void operator=(GlTexture &&) = delete;
};
