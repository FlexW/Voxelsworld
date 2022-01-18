#include "world.hpp"
#include "aabb.hpp"
#include "application.hpp"
#include "block.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "debug_draw.hpp"
#include "defer.hpp"
#include "gl/gl_framebuffer.hpp"
#include "gl/gl_texture.hpp"
#include "gl/gl_texture_array.hpp"
#include "gui_texture.hpp"
#include "image.hpp"

#include <FastDelegate.h>
#include <stb_image.h>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>

using namespace fastdelegate;

namespace
{
glm::ivec3
player_position_to_world_block_position(const glm::vec3 &player_position)
{
  glm::ivec3 block_position{player_position.x,
                            player_position.y,
                            player_position.z};
  if (player_position.x < 0)
  {
    block_position.x -= 1;
  }
  if (player_position.y < 0)
  {
    block_position.y -= 1;
  }
  if (player_position.z < 0)
  {
    block_position.z -= 1;
  }

  return block_position;
}

std::pair<glm::ivec3, glm::ivec3>
world_position_to_chunk_position(const glm::ivec3 &world_position)
{
  glm::ivec3 chunk_position{world_position.x / Chunk::width(),
                            world_position.y / Chunk::height(),
                            world_position.z / Chunk::width()};

  glm::ivec3 block_position{
      world_position.x - chunk_position.x * Chunk::width(),
      world_position.y - chunk_position.y * Chunk::height(),
      world_position.z - chunk_position.z * Chunk::width()};

  if (block_position.x < 0)
  {
    block_position.x += Chunk::width();
  }
  if (block_position.y < 0)
  {
    block_position.y += Chunk::height();
  }
  if (block_position.z < 0)
  {
    block_position.z += Chunk::width();
  }

  if (world_position.x < 0 && world_position.x % Chunk::width() != 0)
  {
    chunk_position.x -= 1;
  }
  if (world_position.y < 0 && world_position.y % Chunk::height() != 0)
  {
    chunk_position.y -= 1;
  }
  if (world_position.z < 0 && world_position.z % Chunk::width() != 0)
  {
    chunk_position.z -= 1;
  }

  return {chunk_position, block_position};
}

glm::ivec3 position_to_chunk_position(const glm::vec3 &position)
{
  const auto block_position = player_position_to_world_block_position(position);

  const auto [chunk_position, _] =
      world_position_to_chunk_position(block_position);
  return chunk_position;
}

} // namespace

World::World()
{
  const auto app    = Application::instance();
  const auto config = app->config();
  grid_size_        = config.config_value_int("World", "grid_size", grid_size_);
  chunks_around_player_ = config.config_value_int("World",
                                                  "chunks_around_player",
                                                  chunks_around_player_);
  debug_sun_ = config.config_value_bool("World", "debug_sun", debug_sun_);

  fog_start_ = config.config_value_float("World", "fog_start", fog_start_);
  fog_end_   = config.config_value_float("World", "fog_end", fog_end_);
  const auto fog_color_r =
      config.config_value_float("World", "sky_color_r", 0.0f);
  const auto fog_color_g =
      config.config_value_float("World", "sky_color_g", 0.0f);
  const auto fog_color_b =
      config.config_value_float("World", "sky_color_b", 0.0f);
  fog_color_ = glm::vec3{fog_color_r, fog_color_g, fog_color_b};

  water_level_ =
      config.config_value_float("Chunk", "water_level", water_level_);

  app->event_manager()->subscribe(
      MakeDelegate(this, &World::on_window_resize_event),
      WindowResizeEvent::id);
}

World::~World()
{
  const auto app = Application::instance();
  app->event_manager()->unsubscribe(
      MakeDelegate(this, &World::on_window_resize_event),
      WindowResizeEvent::id);
}

void World::init()
{
  chunks_.resize(grid_size_);
  for (std::size_t i = 0; i < chunks_.size(); ++i)
  {
    chunks_[i].resize(grid_size_);
  }

  const Image grass_top_image{"data/grass_top.png"};
  assert(grass_top_image.channels_count() == 4);

  const Image grass_side_image{"data/grass_side.png"};
  assert(grass_side_image.channels_count() == 4);

  const Image dirt_image{"data/dirt.png"};
  assert(dirt_image.channels_count() == 4);

  const Image water_image{"data/water.png"};
  assert(water_image.channels_count() == 4);

  const Image log_oak_image{"data/log_oak.png"};
  assert(log_oak_image.channels_count() == 4);

  const Image log_oak_top_image{"data/log_oak_top.png"};
  assert(log_oak_top_image.channels_count() == 4);

  const Image oak_leaves_image{"data/leaves_oak.png"};
  assert(oak_leaves_image.channels_count() == 4);

  block_textures_ = std::make_unique<GlTextureArray>();
  block_textures_->set_data({
      // Order must match block_texture_index()
      {
          grass_top_image.data(),
          grass_top_image.width(),
          grass_top_image.height(),
      },
      {
          grass_side_image.data(),
          grass_side_image.width(),
          grass_side_image.height(),
      },
      {
          dirt_image.data(),
          dirt_image.width(),
          dirt_image.height(),
      },
      {
          water_image.data(),
          water_image.width(),
          water_image.height(),
      },
      {
          log_oak_image.data(),
          log_oak_image.width(),
          log_oak_image.height(),
      },
      {
          log_oak_top_image.data(),
          log_oak_top_image.width(),
          log_oak_top_image.height(),
      },
      {
          oak_leaves_image.data(),
          oak_leaves_image.width(),
          oak_leaves_image.height(),
      },
  });

  world_shader_ = std::make_unique<GlShader>();
  world_shader_->init("shaders/blinn_phong.vert", "shaders/blinn_phong.frag");

  water_shader_ = std::make_unique<GlShader>();
  water_shader_->init("shaders/water.vert", "shaders/water.frag");

  reflection_gui_texture_ = std::make_shared<GuiTexture>();
  refraction_gui_texture_ = std::make_shared<GuiTexture>();
  refraction_gui_texture_->set_position(glm::vec2{400.0f, 0.0f});

  const auto gui = Application::instance()->gui();
  gui->add_gui_element(reflection_gui_texture_);
  gui->add_gui_element(refraction_gui_texture_);

  recreate_framebuffer();
}

int World::block_texture_index(Block::Type block_type,
                               Block::Side block_side) const
{
  switch (block_type)
  {
  case Block::Type::Grass:
  {
    switch (block_side)
    {
    case Block::Side::Top:
      return 0;
    case Block::Side::Bottom:
      return 2;
    case Block::Side::Front:
    case Block::Side::Back:
    case Block::Side::Left:
    case Block::Side::Right:
      return 1;
    }
    assert(0);
    return 0;
  }
  case Block::Type::Dirt:
  {
    return 2;
  }
  case Block::Type::Water:
  {
    return 3;
  }
  case Block::Type::Oak:
  {
    switch (block_side)
    {
    case Block::Side::Top:
      return 5;
    case Block::Side::Bottom:
      return 5;
    case Block::Side::Front:
    case Block::Side::Back:
    case Block::Side::Left:
    case Block::Side::Right:
      return 4;
    }
    assert(0);
    return 0;
  }
  case Block::Type::OakLeaves:
  {
    return 6;
  }
  case Block::Type::Air:
    assert(0);
    return {};
  }
  assert(0);
  return {};
}

void World::set_player_position(const glm::vec3 &position)
{
  player_position_ = position;

  // Generate chunks if needed
  const auto current_chunk_position = position_to_chunk_position(position);

  std::vector<glm::ivec3> need_mesh_generation;

  for (int x = current_chunk_position.x - chunks_around_player_;
       x <= current_chunk_position.x + chunks_around_player_;
       ++x)
  {
    for (int z = current_chunk_position.z - chunks_around_player_;
         z <= current_chunk_position.z + chunks_around_player_;
         ++z)
    {
      // FIXME: This will fail for border chunks
      if (!is_chunk(glm::vec3{x, current_chunk_position.y, z}))
      {
        continue;
      }

      auto &c = chunk(glm::ivec3{x, 0, z});
      if (!c.is_generated())
      {
        const glm::ivec3 pos{x, 0, z};
        c.generate(pos, *this);
        need_mesh_generation.emplace_back(pos);
      }
    }
  }

  for (const auto &pos : need_mesh_generation)
  {
    auto &c = chunk(pos);
    c.generate_mesh(*this);

    // Regenarate neighbours if needed
    {
      const glm::ivec3 neighbour_chunk_pos{pos.x - 1, pos.y, pos.z};
      if (is_chunk(neighbour_chunk_pos))
      {
        auto &c = chunk(neighbour_chunk_pos);
        if (c.is_generated() && c.is_mesh_generated())
        {
          c.regenerate_mesh(*this);
        }
      }
    }
    {
      const glm::ivec3 neighbour_chunk_pos{pos.x, pos.y, pos.z - 1};
      if (is_chunk(neighbour_chunk_pos))
      {
        auto &c = chunk(neighbour_chunk_pos);
        if (c.is_generated() && c.is_mesh_generated())
        {
          c.regenerate_mesh(*this);
        }
      }
    }
    {
      const glm::ivec3 neighbour_chunk_pos{pos.x + 1, pos.y, pos.z};
      if (is_chunk(neighbour_chunk_pos))
      {
        auto &c = chunk(neighbour_chunk_pos);
        if (c.is_generated() && c.is_mesh_generated())
        {
          c.regenerate_mesh(*this);
        }
      }
    }
    {
      const glm::ivec3 neighbour_chunk_pos{pos.x, pos.y, pos.z + 1};
      if (is_chunk(neighbour_chunk_pos))
      {
        auto &c = chunk(neighbour_chunk_pos);
        if (c.is_generated() && c.is_mesh_generated())
        {
          c.regenerate_mesh(*this);
        }
      }
    }
  }
  need_mesh_generation.clear();
}

Chunk &World::chunk_under_position(const glm::vec3 &position)
{
  const auto chunk_position = position_to_chunk_position(position);
  return chunk(chunk_position);
}

const Chunk &World::chunk_under_position(const glm::vec3 &position) const
{
  const auto chunk_position = position_to_chunk_position(position);
  return chunk(chunk_position);
}

glm::ivec3
World::chunk_position_to_storage_position(const glm::ivec3 &position) const
{
  const auto half_grid_size = grid_size_ / 2;

  const auto real_x = position.x + half_grid_size;
  const auto real_z = position.z + half_grid_size;

  return {real_x, position.y, real_z};
}

bool World::is_chunk(const glm::ivec3 &position) const
{
  const auto storage_position = chunk_position_to_storage_position(position);

  return ((storage_position.y == 0) &&
          (0 <= storage_position.x &&
           storage_position.x < static_cast<int>(chunks_.size())) &&
          (0 <= storage_position.z &&
           storage_position.z <
               static_cast<int>(chunks_[storage_position.x].size())));
}

Chunk &World::chunk(const glm::ivec3 &position)
{
  assert(is_chunk(position));
  const auto storage_position = chunk_position_to_storage_position(position);
  return chunks_[storage_position.x][storage_position.z];
}

const Chunk &World::chunk(const glm::ivec3 &position) const
{
  assert(is_chunk(position));
  const auto storage_position = chunk_position_to_storage_position(position);
  return chunks_[storage_position.x][storage_position.z];
}

void World::draw_blocks(const glm::mat4 &view_matrix,
                        const glm::mat4 &projection_matrix,
                        const glm::vec4 &clip_plane)
{
  world_shader_->bind();
  world_shader_->set_uniform("model_matrix", glm::mat4(1.0f));
  world_shader_->set_uniform("view_matrix", view_matrix);
  world_shader_->set_uniform("projection_matrix", projection_matrix);
  world_shader_->set_uniform("clip_plane", clip_plane);

  // Lights
  world_shader_->set_uniform(
      "directional_light.direction",
      glm::vec3{view_matrix * glm::vec4{sun_direction_, 0.0f}});
  world_shader_->set_uniform("directional_light.ambient_color",
                             sun_ambient_color_);
  world_shader_->set_uniform("directional_light.diffuse_color",
                             sun_diffuse_color_);
  world_shader_->set_uniform("directional_light.specular_color",
                             sun_specular_color_);

  // Fog
  world_shader_->set_uniform("fog_start", fog_start_);
  world_shader_->set_uniform("fog_end", fog_end_);
  world_shader_->set_uniform("fog_color", fog_color_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, block_textures_->id());

  // First draw solid blocks
  for (std::size_t x = 0; x < chunks_.size(); ++x)
  {
    for (std::size_t z = 0; z < chunks_[x].size(); ++z)
    {
      // FIXME: This will fail for border chunks
      auto &c = chunks_[x][z];
      if (!c.is_mesh_generated())
      {
        continue;
      }
      const auto chunk_position = c.position();
      // Set model matrix
      const auto chunk_model_matrix =
          glm::translate(glm::mat4(1.0f),
                         glm::vec3(chunk_position.x * Chunk::width(),
                                   0,
                                   chunk_position.z * Chunk::width()));
      world_shader_->set_uniform("model_matrix", chunk_model_matrix);

      c.draw(*world_shader_);
    }
  }

  world_shader_->unbind();
}

void World::draw_water(const glm::mat4 &view_matrix,
                       const glm::mat4 &projection_matrix)
{
  water_shader_->bind();
  water_shader_->set_uniform("model_matrix", glm::mat4(1.0f));
  water_shader_->set_uniform("view_matrix", view_matrix);
  water_shader_->set_uniform("projection_matrix", projection_matrix);

  // Lights
  water_shader_->set_uniform(
      "directional_light.direction",
      glm::vec3{view_matrix * glm::vec4{sun_direction_, 0.0f}});
  water_shader_->set_uniform("directional_light.ambient_color",
                             sun_ambient_color_);
  water_shader_->set_uniform("directional_light.diffuse_color",
                             sun_diffuse_color_);
  water_shader_->set_uniform("directional_light.specular_color",
                             sun_specular_color_);

  // Fog
  water_shader_->set_uniform("fog_start", fog_start_);
  water_shader_->set_uniform("fog_end", fog_end_);
  water_shader_->set_uniform("fog_color", fog_color_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, block_textures_->id());

  // Then draw transparent water
  for (std::size_t x = 0; x < chunks_.size(); ++x)
  {
    for (std::size_t z = 0; z < chunks_[x].size(); ++z)
    {
      // FIXME: This will fail for border chunks
      auto &c = chunks_[x][z];
      if (!c.is_mesh_generated())
      {
        continue;
      }
      const auto chunk_position = c.position();
      // Set model matrix
      const auto chunk_model_matrix =
          glm::translate(glm::mat4(1.0f),
                         glm::vec3(chunk_position.x * Chunk::width(),
                                   0,
                                   chunk_position.z * Chunk::width()));
      water_shader_->set_uniform("model_matrix", chunk_model_matrix);

      c.draw_water(*water_shader_);
    }
  }
}

void World::draw(const Camera    &camera,
                 const glm::mat4 &projection_matrix,
                 DebugDraw       &debug_draw)
{
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Solid Pass");
  draw_blocks(camera.view_matrix(), projection_matrix);
  glPopDebugGroup();

  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Reflection Pass");
  {
    Camera     reflection_camera{camera};
    const auto distance =
        2.0f * (reflection_camera.position().y - water_level_);
    reflection_camera.set_position(
        glm::vec3{reflection_camera.position().x,
                  reflection_camera.position().y - distance,
                  reflection_camera.position().z});
    reflection_camera.set_pitch(-reflection_camera.pitch());
    reflection_framebuffer_->bind();
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glClearColor(fog_color_.r, fog_color_.g, fog_color_.b, 1.0f);
    const auto app = Application::instance();
    glViewport(0, 0, app->window_width(), app->window_height());
    draw_blocks(reflection_camera.view_matrix(),
                projection_matrix,
                glm::vec4{0.0f, 1.0f, 0.0f, -water_level_});
    reflection_framebuffer_->unbind();
  }
  glPopDebugGroup();

  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Refraction Pass");
  refraction_framebuffer_->bind();
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glClearColor(fog_color_.r, fog_color_.g, fog_color_.b, 1.0f);
  const auto app = Application::instance();
  glViewport(0, 0, app->window_width(), app->window_height());
  draw_blocks(camera.view_matrix(),
              projection_matrix,
              glm::vec4{0.0f, -1.0f, 0.0f, water_level_});
  refraction_framebuffer_->unbind();
  glPopDebugGroup();

  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Water Pass");
  draw_water(camera.view_matrix(), projection_matrix);
  glPopDebugGroup();

  if (debug_sun_)
  {
    debug_draw.draw_line(glm::vec3{0.0f, 50.0f, 0.0f},
                         glm::vec3{0.0f, 50.0f, 0.0f} + sun_direction_ * 800.0f,
                         glm::vec3{1.0f, 1.0f, 0.0f});
  }
}

bool World::is_chunk_under_position(const glm::ivec3 &world_position) const
{
  const auto [chunk_position, _] =
      world_position_to_chunk_position(world_position);

  return is_chunk(chunk_position);
}

bool World::is_block(const glm::ivec3 &world_position) const
{
  const auto [chunk_position, block_position] =
      world_position_to_chunk_position(world_position);

  if (!is_chunk(chunk_position))
  {
    return false;
  }

  auto      &c          = chunk(chunk_position);
  const auto block_type = c.block_type(block_position);

  return block_type != Block::Type::Air;
}

[[nodiscard]] bool World::is_block(const glm::ivec3 &world_position,
                                   Block::Type       type) const
{
  const auto [chunk_position, block_position] =
      world_position_to_chunk_position(world_position);

  if (!is_chunk(chunk_position))
  {
    return false;
  }

  auto      &c          = chunk(chunk_position);
  const auto block_type = c.block_type(block_position);

  if (type == Block::Type::Water)
  {
    return block_type != Block::Type::Air;
  }

  return block_type != Block::Type::Air && block_type != Block::Type::Water;
}

bool World::remove_block(const glm::vec3 &position)
{
  const auto block_position = player_position_to_world_block_position(position);

  if (!is_chunk_under_position(block_position))
  {
    return false;
  }

  const auto [chunk_position, block_in_chunk_position] =
      world_position_to_chunk_position(block_position);

  auto &c = chunk(chunk_position);

  return c.remove_block(*this, block_in_chunk_position);
}

bool World::place_block(const Ray &ray)
{
  const auto block_position =
      player_position_to_world_block_position(ray.end());

  if (!is_chunk_under_position(block_position))
  {
    return false;
  }

  const auto [chunk_position, block_in_chunk_position] =
      world_position_to_chunk_position(block_position);

  auto &c = chunk(chunk_position);
  if (c.block_type(block_in_chunk_position) == Block::Type::Air)
  {
    return false;
  }

  Aabb       aabb{block_position,
            glm::vec3{block_position.x + Block::width,
                      block_position.y + Block::height,
                      block_position.z + Block::width}};
  const auto intersection = aabb.intersect(ray);
  if (intersection.has_value())
  {
    const auto new_block_world_position =
        player_position_to_world_block_position(intersection.value());

    const auto [_, new_block_chunk_position] =
        world_position_to_chunk_position(new_block_world_position);

    c.place_block(*this, new_block_chunk_position, Block::Type::Grass);

    return true;
  }

  return false;
}

void World::regenerate_chunk(const glm::ivec3 &chunk_position)
{
  if (!is_chunk(chunk_position))
  {
    return;
  }
  auto &c = chunk(chunk_position);
  c.regenerate_mesh(*this);
}

void World::on_window_resize_event(std::shared_ptr<Event> /*event*/)
{
  recreate_framebuffer();
}

void World::recreate_framebuffer()
{
  // Create the reflection framebuffer
  {
    reflection_framebuffer_  = std::make_unique<GlFramebuffer>();
    const auto app           = Application::instance();
    const auto window_width  = app->window_width();
    const auto window_height = app->window_height();

    FramebufferAttachment color_attachment{};
    color_attachment.type_            = AttachmentType::Texture;
    color_attachment.format_          = GL_RGBA;
    color_attachment.internal_format_ = GL_RGBA8;
    color_attachment.width_           = window_width;
    color_attachment.height_          = window_height;

    FramebufferAttachment depth_attachment{};
    depth_attachment.type_            = AttachmentType::Renderbuffer;
    depth_attachment.internal_format_ = GL_DEPTH_COMPONENT24;
    depth_attachment.width_           = window_width;
    depth_attachment.height_          = window_height;

    FramebufferConfig framebuffer_config{};
    framebuffer_config.color_attachments_.push_back(color_attachment);
    framebuffer_config.depth_attachment_ = depth_attachment;

    reflection_framebuffer_->attach(framebuffer_config);
  }

  // Create the refraction framebuffer
  {
    refraction_framebuffer_  = std::make_unique<GlFramebuffer>();
    const auto app           = Application::instance();
    const auto window_width  = app->window_width();
    const auto window_height = app->window_height();

    FramebufferAttachment color_attachment{};
    color_attachment.type_            = AttachmentType::Texture;
    color_attachment.format_          = GL_RGBA;
    color_attachment.internal_format_ = GL_RGBA8;
    color_attachment.width_           = window_width;
    color_attachment.height_          = window_height;

    FramebufferAttachment depth_attachment{};
    depth_attachment.type_            = AttachmentType::Renderbuffer;
    depth_attachment.internal_format_ = GL_DEPTH_COMPONENT24;
    depth_attachment.width_           = window_width;
    depth_attachment.height_          = window_height;

    FramebufferConfig framebuffer_config{};
    framebuffer_config.color_attachments_.push_back(color_attachment);
    framebuffer_config.depth_attachment_ = depth_attachment;

    refraction_framebuffer_->attach(framebuffer_config);
  }

  auto reflection_texture = std::get<std::shared_ptr<GlTexture>>(
      reflection_framebuffer_->color_attachment(0));
  reflection_gui_texture_->set_texture(reflection_texture);

  auto refraction_texture = std::get<std::shared_ptr<GlTexture>>(
      refraction_framebuffer_->color_attachment(0));
  refraction_gui_texture_->set_texture(refraction_texture);
}
