#pragma once

#include "gl/gl_texture.hpp"
#include "math.hpp"

#include <filesystem>
#include <memory>

class TextureAtlas
{
public:
  struct Coords
  {
    glm::vec2 top_left;
    glm::vec2 top_right;
    glm::vec2 bottom_left;
    glm::vec2 bottom_right;
  };

  GLuint texture_id() const;

  bool load(const std::filesystem::path &file_path,
            int                          texture_width_count,
            int                          texture_height_count);

  Coords get(int texture_width_index, int texture_height_index) const;

private:
  std::unique_ptr<GlTexture> texture_ = std::make_unique<GlTexture>();

  int texture_width_{};
  int texture_height_{};

  float sub_texture_width_{};
  float sub_texture_height_{};
};
