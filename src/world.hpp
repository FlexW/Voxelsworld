#pragma once

#include "block.hpp"
#include "chunk.hpp"
#include "debug_draw.hpp"
#include "event.hpp"
#include "gl/gl_framebuffer.hpp"
#include "gl/gl_renderbuffer.hpp"
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
  ~World();

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

  std::unique_ptr<GlTextureArray> block_textures_{};
  std::unique_ptr<GlFramebuffer>  framebuffer_{};

  std::unique_ptr<GlShader> world_shader_{};
  std::unique_ptr<GlShader> water_shader_{};

  float     fog_start_;
  float     fog_end_;
  glm::vec3 fog_color_;

  glm::vec3 sun_direction_{glm::normalize(glm::vec3{-7.0f, -8.8, 0.0f})};
  glm::vec3 sun_ambient_color_{0.6f};
  glm::vec3 sun_diffuse_color_{1.0f};
  glm::vec3 sun_specular_color_{0.8f};

  bool is_chunk(const glm::ivec3 &position) const;

  glm::ivec3
  chunk_position_to_storage_position(const glm::ivec3 &position) const;

  bool is_chunk_under_position(const glm::ivec3 &world_positon) const;

  Chunk       &chunk_under_position(const glm::vec3 &position);
  const Chunk &chunk_under_position(const glm::vec3 &position) const;

  Chunk       &chunk(const glm::ivec3 &position);
  const Chunk &chunk(const glm::ivec3 &position) const;

  void draw_blocks(const glm::mat4 &view_matrix,
                   const glm::mat4 &projection_matrix);
  void draw_water(const glm::mat4 &view_matrix,
                  const glm::mat4 &projection_matrix);

  void on_window_resize_event(std::shared_ptr<Event> event);

  void recreate_framebuffer();
};
