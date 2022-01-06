#pragma once

#include "block.hpp"
#include "gl/gl_index_buffer.hpp"
#include "gl/gl_shader.hpp"
#include "gl/gl_vertex_buffer.hpp"

#include <array>
#include <memory>

class World;

class Chunk
{
public:
  static constexpr int height = 256;
  static constexpr int width  = 16;

  [[nodiscard]] bool is_generated() const;
  void               generate(const glm::vec3 &position, const World &world);

  [[nodiscard]] bool is_mesh_generated() const;

  void generate_mesh(const World &world);
  void regenerate_mesh(const World &world);

  void draw(GlShader &shader);

  glm::ivec3 position() const;

  Block::Type block_type(int x, int y, int z) const;

private:
  std::vector<const GlVertexBuffer *> vertex_buffers_;
  std::array<std::array<std::array<Block, height>, width>, width> blocks_;

  glm::ivec3 position_{};

  bool is_generated_      = false;
  bool is_mesh_generated_ = false;

  std::unique_ptr<GlVertexBuffer> vertex_buffer_positions_{};
  std::unique_ptr<GlVertexBuffer> vertex_buffer_normals_{};
  std::unique_ptr<GlVertexBuffer> vertex_buffer_tex_coords_{};

  std::unique_ptr<GlIndexBuffer> index_buffer_{};

  [[nodiscard]] bool is_block(int x, int y, int z) const;
  [[nodiscard]] bool is_block(int x, int y, int z, const World &world) const;

  void fill_mesh_data(const World &world);
};
