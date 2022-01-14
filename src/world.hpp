#pragma once

#include "block.hpp"
#include "chunk.hpp"
#include "debug_draw.hpp"
#include "gl/gl_shader.hpp"
#include "gl/gl_texture.hpp"
#include "gl/gl_texture_array.hpp"
#include "math.hpp"
#include "ray.hpp"

#include <array>
#include <memory>

class World
{
public:
  World();

  void init();

  void set_player_position(const glm::vec3 &position);

  void draw(const glm::mat4 &view_matrix,
            const glm::mat4 &projection_matrix,
            DebugDraw       &debug_draw);

  [[nodiscard]] bool is_block(const glm::ivec3 &world_position) const;
  [[nodiscard]] bool is_block(const glm::ivec3 &world_position,
                              Block::Type       type) const;

  bool remove_block(const glm::vec3 &position);
  bool place_block(const Ray &ray);

  void regenerate_chunk(const glm::ivec3 &chunk_position);

  int block_texture_index(Block::Type block_type, Block::Side block_side) const;

private:
  int grid_size_            = 64;
  int chunks_around_player_ = 16;

  bool debug_sun_ = false;

  std::vector<std::vector<Chunk>> chunks_;

  glm::vec3 player_position_ = glm::vec3(0.0f);

  std::unique_ptr<GlTextureArray> block_textures_;

  std::unique_ptr<GlShader> world_shader_{};
  std::unique_ptr<GlShader> water_shader_{};

  bool is_chunk(const glm::ivec3 &position) const;

  glm::ivec3
  chunk_position_to_storage_position(const glm::ivec3 &position) const;

  bool is_chunk_under_position(const glm::ivec3 &world_positon) const;

  Chunk       &chunk_under_position(const glm::vec3 &position);
  const Chunk &chunk_under_position(const glm::vec3 &position) const;

  Chunk       &chunk(const glm::ivec3 &position);
  const Chunk &chunk(const glm::ivec3 &position) const;
};
