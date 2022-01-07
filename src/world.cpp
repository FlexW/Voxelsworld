#include "world.hpp"
#include "block.hpp"
#include "chunk.hpp"
#include "defer.hpp"
#include "gl/gl_texture.hpp"

#include <memory>
#include <stb_image.h>

#include <cassert>

namespace
{
constexpr int t = 4;

std::pair<int, int> position_to_chunk_position(const glm::vec3 &position)
{
  auto x = position.x;
  auto z = position.z;
  if (x < 0)
  {
    x = std::ceil(std::abs(x) / static_cast<float>(Chunk::width));
    x *= -1.0f;
  }
  else
  {
    x = std::floor(x / static_cast<float>(Chunk::width));
  }
  if (z < 0)
  {
    z = std::ceil(std::abs(z) / static_cast<float>(Chunk::width));
    z *= -1.0f;
  }
  else
  {
    z = std::floor(z / static_cast<float>(Chunk::width));
  }

  return {static_cast<int>(x), static_cast<int>(z)};
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
  const auto [current_chunk_position_x, current_chunk_position_z] =
      position_to_chunk_position(position);

  std::vector<glm::ivec2> need_mesh_generation;

  for (int x = current_chunk_position_x - t; x <= current_chunk_position_x + t;
       ++x)
  {
    for (int z = current_chunk_position_z - t;
         z <= current_chunk_position_z + t;
         ++z)
    {
      // FIXME: This will fail for border chunks
      auto &c = chunk(x, z);
      if (!c.is_generated())
      {
        c.generate(glm::vec3(x, 0, z), *this);
        need_mesh_generation.emplace_back(x, z);
      }
    }
  }

  for (const auto &pos : need_mesh_generation)
  {
    auto &c = chunk(pos.x, pos.y);
    c.generate_mesh(*this);

    // Regenarate neigbours if needed
    {
      auto &c = chunk(pos.x - 1, pos.y);
      if (c.is_generated() && c.is_mesh_generated())
      {
        c.regenerate_mesh(*this);
      }
    }
    {
      auto &c = chunk(pos.x, pos.y - 1);
      if (c.is_generated() && c.is_mesh_generated())
      {
        c.regenerate_mesh(*this);
      }
    }
    {
      auto &c = chunk(pos.x + 1, pos.y);
      if (c.is_generated() && c.is_mesh_generated())
      {
        c.regenerate_mesh(*this);
      }
    }
    {
      auto &c = chunk(pos.x, pos.y + 1);
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
  const auto [x, z] = position_to_chunk_position(position);
  return chunk(x, z);
}

const Chunk &World::chunk_under_position(const glm::vec3 &position) const
{
  const auto [x, z] = position_to_chunk_position(position);
  return chunk(x, z);
}

Chunk &World::chunk(int x, int z)
{
  static constexpr int half_grid_size = grid_size / 2;

  const auto real_x = x + half_grid_size;
  const auto real_z = z + half_grid_size;

  assert(0 <= real_x < chunks_.size());
  assert(0 <= real_z < chunks_[x].size());

  return chunks_[real_x][real_z];
}

const Chunk &World::chunk(int x, int z) const
{
  static constexpr int half_grid_size = grid_size / 2;

  const auto real_x = x + half_grid_size;
  const auto real_z = z + half_grid_size;

  assert(0 <= real_x < chunks_.size());
  assert(0 <= real_z < chunks_[x].size());

  return chunks_[real_x][real_z];
}

void World::draw(GlShader &shader)
{

  const auto [current_chunk_position_x, current_chunk_position_z] =
      position_to_chunk_position(player_position_);

  for (int x = current_chunk_position_x - t; x <= current_chunk_position_x + t;
       ++x)
  {
    for (int z = current_chunk_position_z - t;
         z <= current_chunk_position_z + t;
         ++z)
    {
      // FIXME: This will fail for border chunks
      auto &c = chunk(x, z);
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

bool World::is_block(int x, int y, int z) const
{
  const auto &chunk = chunk_under_position(glm::vec3(x, y, z));
  if (!chunk.is_generated())
  {
    return false;
  }

  const auto chunk_position = chunk.position();
  const auto cx             = chunk_position.x * Chunk::width;
  const auto cz             = chunk_position.z * Chunk::width;
  x                         = x - cx;
  z                         = z - cz;

  const auto block_type = chunk.block_type(x, y, z);
  return block_type != Block::Type::Air;
}
