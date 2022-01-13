#include "world.hpp"
#include "aabb.hpp"
#include "application.hpp"
#include "block.hpp"
#include "chunk.hpp"
#include "defer.hpp"
#include "gl/gl_texture.hpp"

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

[[nodiscard]] std::unique_ptr<GlTexture>
load_texture(const std::filesystem::path &file_path)
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
    return {};
  }
  defer(stbi_image_free(texture_data));

  auto texture = std::make_unique<GlTexture>();
  texture->set_data(texture_data, width, height, channels_count);

  return texture;
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

  const auto result = world_texure_atlas_.load("data/world_texture.png", 4, 1);
  if (!result)
  {
    std::cerr << "Could not load world texture" << std::endl;
    assert(0);
  }
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
      shader.set_uniform("is_diffuse_tex", true);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, world_texure_atlas_.texture_id());
      shader.set_uniform("in_diffuse_tex",
                         static_cast<int>(world_texure_atlas_.texture_id()));
      glBindTexture(GL_TEXTURE_2D, 0);
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

TextureAtlas::Coords World::world_texture_coords(int texture_width_index,
                                                 int texture_height_index) const
{
  return world_texure_atlas_.get(texture_width_index, texture_height_index);
}
