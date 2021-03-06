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
  static int width();
  static int height();

  Chunk();

  [[nodiscard]] bool is_generated() const;
  void               generate(const glm::vec3 &position, const World &world);

  [[nodiscard]] bool is_mesh_generated() const;

  void generate_mesh(const World &world);
  void regenerate_mesh(const World &world);

  void draw(GlShader &shader);
  void draw_water(GlShader &shader);

  glm::ivec3 position() const;

  Block::Type block_type(const glm::ivec3 &position) const;

  bool remove_block(World &world, const glm::ivec3 &position);
  bool
  place_block(World &world, const glm::ivec3 &position, Block::Type block_type);

private:
  std::vector<const GlVertexBuffer *>          vertex_buffers_;
  std::vector<const GlVertexBuffer *>          water_vertex_buffers_;
  std::vector<std::vector<std::vector<Block>>> blocks_;

  glm::ivec3 position_{};

  float c1_;
  float c2_;
  float c3_;
  float div_;
  float frequency1_;
  float frequency2_;
  float frequency3_;
  float e_;
  float fudge_factor_;
  float water_level_ = 5.0f;
  float terraces_;

  int min_tree_height_;
  int max_tree_height_;

  int min_leaves_radius_;
  int max_leaves_radius_;

  int tree_density_;
  int leave_density_;

  bool is_generated_      = false;
  bool is_mesh_generated_ = false;

  std::unique_ptr<GlVertexBuffer> vertex_buffer_positions_{};
  std::unique_ptr<GlVertexBuffer> vertex_buffer_normals_{};
  std::unique_ptr<GlVertexBuffer> vertex_buffer_tex_coords_{};
  std::unique_ptr<GlVertexBuffer> vertex_buffer_tex_indices_{};

  std::unique_ptr<GlVertexBuffer> vertex_buffer_water_positions_{};
  std::unique_ptr<GlVertexBuffer> vertex_buffer_water_normals_{};
  std::unique_ptr<GlVertexBuffer> vertex_buffer_water_tex_coords_{};
  std::unique_ptr<GlVertexBuffer> vertex_buffer_water_tex_indices_{};

  std::unique_ptr<GlIndexBuffer> index_buffer_{};
  std::unique_ptr<GlIndexBuffer> water_index_buffer_{};

  [[nodiscard]] bool is_block(const glm::ivec3 &position,
                              Block::Type       type) const;
  [[nodiscard]] bool is_block(const glm::ivec3 &position,
                              const World      &world,
                              Block::Type       type) const;

  void fill_mesh_data(const World &world);

  bool is_valid_block_position(const glm::ivec3 &position) const;

  Block &block(const glm::ivec3 &position);

  glm::ivec3
  block_position_to_world_position(const glm::ivec3 &block_position) const;

  void regenerate_chunks_if_border_block(World            &world,
                                         const glm::ivec3 &position);

  void send_mesh_data_to_gpu(const std::vector<glm::vec3> &block_positions,
                             const std::vector<glm::vec3> &block_normals,
                             const std::vector<glm::vec2> &block_tex_coords,
                             const std::vector<int>       &block_tex_indices,
                             const std::vector<unsigned>  &block_indices,
                             const std::vector<glm::vec3> &water_positions,
                             const std::vector<glm::vec3> &water_normals,
                             const std::vector<glm::vec2> &water_tex_coords,
                             const std::vector<int>       &water_tex_indices,
                             const std::vector<unsigned>  &water_indices);

  void generate_mesh_data(const World            &world,
                          std::vector<glm::vec3> &block_positions,
                          std::vector<glm::vec3> &block_normals,
                          std::vector<glm::vec2> &block_tex_coords,
                          std::vector<int>       &block_tex_indices,
                          std::vector<unsigned>  &block_indices,
                          std::vector<glm::vec3> &water_positions,
                          std::vector<glm::vec3> &water_normals,
                          std::vector<glm::vec2> &water_tex_coords,
                          std::vector<int>       &water_tex_indices,
                          std::vector<unsigned>  &water_indices);
};
