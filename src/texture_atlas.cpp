#include "texture_atlas.hpp"
#include "defer.hpp"

#include <iostream>
#include <stb_image.h>

GLuint TextureAtlas::texture_id() const
{
  return texture_ ? texture_->id() : 0;
}

bool TextureAtlas::load(const std::filesystem::path &file_path,
                        int                          texture_width_count,
                        int                          texture_height_count)
{
  int width = 0, height = 0, channels_count = 0;
  stbi_set_flip_vertically_on_load(true);
  auto texture_data = stbi_load(file_path.string().c_str(),
                                &width,
                                &height,
                                &channels_count,
                                0);
  if (!texture_data)
  {
    std::cerr << "Error: Could not load texture " << file_path.string()
              << std::endl;
    return false;
  }
  defer(stbi_image_free(texture_data));

  texture_ = std::make_unique<GlTexture>();
  texture_->set_data(texture_data, width, height, channels_count);

  texture_width_  = width;
  texture_height_ = height;

  sub_texture_width_ = static_cast<float>(texture_width_) /
                       static_cast<float>(texture_width_count);
  sub_texture_height_ = static_cast<float>(texture_height_) /
                        static_cast<float>(texture_height_count);

  return true;
}

TextureAtlas::Coords TextureAtlas::get(int texture_width_index,
                                       int texture_height_index) const
{
  const auto start_x = texture_width_index * sub_texture_width_;
  const auto end_x   = start_x + sub_texture_width_;
  const auto start_y = texture_height_index * sub_texture_height_;
  const auto end_y   = start_y + sub_texture_height_;

  assert(0.0f <= start_x && start_x <= static_cast<float>(texture_width_));
  assert(0.0f <= end_x && end_x <= static_cast<float>(texture_width_));
  assert(0.0f <= start_y && start_y <= static_cast<float>(texture_height_));
  assert(0.0f <= end_y && end_y <= static_cast<float>(texture_height_));

  Coords coords{};
  coords.top_left =
      glm::vec2{start_x / texture_width_, start_y / texture_height_};
  coords.top_right =
      glm::vec2{end_x / texture_width_, start_y / texture_height_};
  coords.bottom_left =
      glm::vec2{start_x / texture_width_, end_y / texture_height_};
  coords.bottom_right =
      glm::vec2{end_x / texture_width_, end_y / texture_height_};

  return coords;
}
