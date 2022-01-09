#include "world.hpp"
#include "aabb.hpp"
#include "block.hpp"
#include "chunk.hpp"
#include "defer.hpp"
#include "gl/gl_texture.hpp"

#include <memory>
#include <stb_image.h>

#include <cassert>

namespace
{
constexpr int t = 8;

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
  glm::ivec3 chunk_position{world_position.x / Chunk::width,
                            world_position.y / Chunk::height,
                            world_position.z / Chunk::width};

  glm::ivec3 block_position{world_position.x - chunk_position.x * Chunk::width,
                            world_position.y - chunk_position.y * Chunk::height,
                            world_position.z - chunk_position.z * Chunk::width};

  if (block_position.x < 0)
  {
    block_position.x += Chunk::width;
  }
  if (block_position.y < 0)
  {
    block_position.y += Chunk::height;
  }
  if (block_position.z < 0)
  {
    block_position.z += Chunk::width;
  }

  if (world_position.x < 0 && world_position.x % Chunk::width != 0)
  {
    chunk_position.x -= 1;
  }
  if (world_position.y < 0 && world_position.y % Chunk::height != 0)
  {
    chunk_position.y -= 1;
  }
  if (world_position.z < 0 && world_position.z % Chunk::width != 0)
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

void World::init()
{
  texture_ = load_texture("data/test_texture.jpg");
  // texture_ = load_texture("data/awesomeface.png");
}

void World::set_player_position(const glm::vec3 &position)
{
  player_position_ = position;

  // Generate chunks if needed
  const auto current_chunk_position = position_to_chunk_position(position);

  std::vector<glm::ivec3> need_mesh_generation;

  for (int x = current_chunk_position.x - t; x <= current_chunk_position.x + t;
       ++x)
  {
    for (int z = current_chunk_position.z - t;
         z <= current_chunk_position.z + t;
         ++z)
    {
      // FIXME: This will fail for border chunks
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
      auto &c = chunk(glm::ivec3{pos.x - 1, pos.y, pos.z});
      if (c.is_generated() && c.is_mesh_generated())
      {
        c.regenerate_mesh(*this);
      }
    }
    {
      auto &c = chunk(glm::ivec3{pos.x, pos.y, pos.z - 1});
      if (c.is_generated() && c.is_mesh_generated())
      {
        c.regenerate_mesh(*this);
      }
    }
    {
      auto &c = chunk(glm::ivec3{pos.x + 1, pos.y, pos.z});
      if (c.is_generated() && c.is_mesh_generated())
      {
        c.regenerate_mesh(*this);
      }
    }
    {
      auto &c = chunk(glm::ivec3{pos.x, pos.y, pos.z + 1});
      if (c.is_generated() && c.is_mesh_generated())
      {
        c.regenerate_mesh(*this);
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
  static constexpr int half_grid_size = grid_size / 2;

  const auto real_x = position.x + half_grid_size;
  const auto real_z = position.z + half_grid_size;

  return {real_x, position.y, real_z};
}

bool World::is_chunk(const glm::ivec3 &position) const
{
  const auto storage_position = chunk_position_to_storage_position(position);

  return ((storage_position.y == 0) &&
          (0 <= storage_position.x < chunks_.size()) &&
          (0 <= storage_position.z < chunks_[storage_position.x].size()));
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

  for (int x = current_chunk_position.x - t; x <= current_chunk_position.x + t;
       ++x)
  {
    for (int z = current_chunk_position.z - t;
         z <= current_chunk_position.z + t;
         ++z)
    {
      // FIXME: This will fail for border chunks
      auto &c = chunk(glm::ivec3{x, 0, z});
      assert(c.is_generated());
      // Set model matrix
      const auto chunk_model_matrix =
          glm::translate(glm::mat4(1.0f),
                         glm::vec3(x * Chunk::width, 0, z * Chunk::width));
      shader.set_uniform("model_matrix", chunk_model_matrix);

      // Material
      // shader.set_uniform("in_diffuse_color", glm::vec3(0.34f, 0.49f, 0.27f));
      shader.set_uniform("is_diffuse_tex", true);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_->id());
      shader.set_uniform("in_diffuse_tex", static_cast<int>(texture_->id()));
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

  auto &c = chunk(chunk_position);
  const auto block_type = c.block_type(block_position);

  return block_type != Block::Type::Air;
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

  Aabb aabb{block_position,
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