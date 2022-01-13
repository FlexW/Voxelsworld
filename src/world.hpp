#pragma once

#include "chunk.hpp"
#include "gl/gl_shader.hpp"
#include "gl/gl_texture.hpp"
#include "math.hpp"
#include "ray.hpp"
#include "texture_atlas.hpp"

#include <array>
#include <memory>

class World
{
public:
  World();

  void init();

  void set_player_position(const glm::vec3 &position);

  void draw(GlShader &shader);

  [[nodiscard]] bool is_block(const glm::ivec3 &world_position) const;

  bool remove_block(const glm::vec3 &position);
  bool place_block(const Ray &ray);

  void regenerate_chunk(const glm::ivec3 &chunk_position);

  TextureAtlas::Coords world_texture_coords(int texture_width_index,
                                            int texture_height_index) const;

private:
  int grid_size_            = 64;
  int chunks_around_player_ = 16;

  std::vector<std::vector<Chunk>> chunks_;

  glm::vec3 player_position_ = glm::vec3(0.0f);

  TextureAtlas world_texure_atlas_;
  // std::unique_ptr<GlTexture> texture_{};

  bool is_chunk(const glm::ivec3 &position) const;

  glm::ivec3
  chunk_position_to_storage_position(const glm::ivec3 &position) const;

  bool is_chunk_under_position(const glm::ivec3 &world_positon) const;

  Chunk       &chunk_under_position(const glm::vec3 &position);
  const Chunk &chunk_under_position(const glm::vec3 &position) const;

  Chunk       &chunk(const glm::ivec3 &position);
  const Chunk &chunk(const glm::ivec3 &position) const;
};
