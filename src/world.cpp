#include "world.hpp"
#include "aabb.hpp"
#include "application.hpp"
#include "block.hpp"
#include "chunk.hpp"
#include "defer.hpp"
#include "gl/gl_texture.hpp"
#include "gl/gl_texture_array.hpp"
#include "image.hpp"

#include <cstdint>
#include <stb_image.h>

#include <cassert>
#include <filesystem>
#include <memory>

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
  });
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
  }
  case Block::Type::Dirt:
  {
    return 2;
  }
  case Block::Type::Water:
  {
    return 3;
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
          (0 <= storage_position.x && storage_position.x < chunks_.size()) &&
          (0 <= storage_position.z &&
           storage_position.z < chunks_[storage_position.x].size()));
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

void World::draw(GlShader &shader)
{

  const auto current_chunk_position =
      position_to_chunk_position(player_position_);
  for (int x = 0; x < chunks_.size(); ++x)
  {
    for (int z = 0; z < chunks_[x].size(); ++z)
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
      shader.set_uniform("model_matrix", chunk_model_matrix);

      // Material
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D_ARRAY, block_textures_->id());
      // shader.set_uniform("in_diffuse_tex",
      //                    static_cast<int>(block_textures_->id()));
      c.draw(shader);
    }
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
    std::cout << "No chunk found under " << position << std::endl;
    return false;
  }

  const auto [chunk_position, block_in_chunk_position] =
      world_position_to_chunk_position(block_position);

  auto &c = chunk(chunk_position);

  std::cout << "Chunk positon: " << c.position() << std::endl;
  std::cout << "Block pos in chunk: " << block_in_chunk_position << std::endl;

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

  std::cout << "Test intersection with block" << block_position << std::endl;

  Aabb       aabb{block_position,
            glm::vec3{block_position.x + Block::width,
                      block_position.y + Block::height,
                      block_position.z + Block::width}};
  const auto intersection = aabb.intersect(ray);
  if (intersection.has_value())
  {
    std::cout << "Place block" << std::endl;
    std::cout << "Intersect: " << intersection.value() << std::endl;
    const auto new_block_world_position =
        player_position_to_world_block_position(intersection.value());
    std::cout << "New block position: " << new_block_world_position
              << std::endl;

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
