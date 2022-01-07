#pragma once

#include "chunk.hpp"
#include "gl/gl_shader.hpp"
#include "gl/gl_texture.hpp"
#include "math.hpp"

#include <array>
#include <memory>

class World
{
public:
  // Should be an even number
  static constexpr int grid_size = 64;

  void init();

  void set_player_position(const glm::vec3 &position);

  void draw(GlShader &shader);

  [[nodiscard]] bool is_block(int x, int y, int z) const;

private:
  std::array<std::array<Chunk, grid_size>, grid_size> chunks_;

  glm::vec3 player_position_ = glm::vec3(0.0f);

  std::unique_ptr<GlTexture> texture_{};

  Chunk       &chunk_under_position(const glm::vec3 &position);
  const Chunk &chunk_under_position(const glm::vec3 &position) const;

  Chunk       &chunk(int x, int z);
  const Chunk &chunk(int x, int z) const;
};
