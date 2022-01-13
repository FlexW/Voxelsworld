#include "chunk.hpp"
#include "application.hpp"
#include "block.hpp"
#include "gl/gl_index_buffer.hpp"
#include "gl/gl_vertex_buffer.hpp"
#include "math.hpp"
#include "texture_atlas.hpp"
#include "world.hpp"

#include <cassert>
#include <limits>
#include <memory>

namespace
{
enum class BlockSide
{
  Top,
  Bottom,
  Left,
  Right,
  Front,
  Back,
};

TextureAtlas::Coords block_type_to_texture_coords(Block::Type  type,
                                                  BlockSide    block_side,
                                                  const World &world)
{
  switch (type)
  {
  case Block::Type::Grass:
  {
    switch (block_side)
    {
    case BlockSide::Top:
      return world.world_texture_coords(2, 0);
    case BlockSide::Bottom:
      return world.world_texture_coords(0, 0);
    case BlockSide::Front:
    case BlockSide::Back:
    case BlockSide::Left:
    case BlockSide::Right:
      return world.world_texture_coords(1, 0);
    }
  }
  case Block::Type::Dirt:
  {
    return world.world_texture_coords(0, 0);
  }
  case Block::Type::Water:
  {
    return world.world_texture_coords(3, 0);
  }
  case Block::Type::Air:
    assert(0);
    return {};
  }
  assert(0);
  return {};
}

} // namespace

int Chunk::width()
{
  static const auto w =
      Application::instance()->config().config_value_int("Chunk", "width", 16);
  return w;
}

int Chunk::height()
{
  static const auto h =
      Application::instance()->config().config_value_int("Chunk",
                                                         "height",
                                                         256);
  return h;
}

Chunk::Chunk()
{
  auto       app    = Application::instance();
  const auto config = app->config();

  c1_           = config.config_value_float("Chunk", "c1", 1.0f);
  c2_ = config.config_value_float("Chunk", "c2", 0.7f);
  c3_  = config.config_value_float("Chunk", "c3", 0.008f);
  div_        = config.config_value_float("Chunk", "div", 1.0f);
  frequency1_ = config.config_value_float("Chunk", "frequency1", 0.0003f);
  frequency2_ = config.config_value_float("Chunk", "frequency2", 0.008f);
  frequency3_ = config.config_value_float("Chunk", "frequency3", 0.1f);
  e_            = config.config_value_float("Chunk", "e", 11.3);
  fudge_factor_ = config.config_value_float("Chunk", "fudge_factor", 1.1);
  water_level_  = config.config_value_float("Chunk", "water_level", 5.0);
  terraces_     = config.config_value_float("Chunk", "terraces", 180.0);

  blocks_.resize(width());
  for (int x = 0; x < blocks_.size(); ++x)
  {
    blocks_[x].resize(width());
    for (int z = 0; z < blocks_[x].size(); ++z)
    {
      blocks_[x][z].resize(height());
    }
  }
}

[[nodiscard]] bool Chunk::is_generated() const { return is_generated_; }

void Chunk::generate(const glm::vec3 &position, const World &world)
{
  if (is_generated_)
  {
    return;
  }

  position_ = glm::ivec3(static_cast<int>(position.x),
                         static_cast<int>(position.y),
                         static_cast<int>(position.z));

  for (int x = 0; x < blocks_.size(); ++x)
  {
    for (int z = 0; z < blocks_[x].size(); ++z)
    {
      const auto world_position =
          block_position_to_world_position(glm::ivec3{x, 0, z});

      auto position_x = world_position.x + 0.5f;
      auto position_z = world_position.z + 0.5f;

      const auto noise =
          c1_ * glm::simplex(glm::vec2{position_x * frequency1_ - 1.3f,
                                       position_z * frequency1_}) +
          c2_ * glm::simplex(glm::vec2{position_x * frequency2_ + 1.1f,
                                       position_z * frequency2_ + 2.0f}) +
          c3_ * glm::simplex(glm::vec2{position_x * frequency3_ + 5.3f,
                                       position_z * frequency3_ + 0.2f});

      auto normalized_noise =
          (noise + c1_ + c2_ + c3_) / (((2.0f * (c1_ + c2_ + c3_))) * div_);

      normalized_noise = glm::pow(normalized_noise * fudge_factor_, e_);
      normalized_noise = glm::round(normalized_noise * terraces_) / terraces_;

      auto height = static_cast<int>(normalized_noise * Chunk::height());

      for (int y = 0; y <= height || y <= water_level_; ++y)
      {
        assert(0 <= y && y < blocks_[x][z].size());
        auto &block = blocks_[x][z][y];
        if (y == height)
        {
          block.set_type(Block::Type::Grass);
        }
        else if (y < height)
        {
          block.set_type(Block::Type::Dirt);
        }
        else if (y <= water_level_)
        {
          block.set_type(Block::Type::Water);
        }
      }
    }
  }

  is_generated_ = true;
}

[[nodiscard]] bool Chunk::is_mesh_generated() const
{
  return is_mesh_generated_;
}

bool Chunk::is_block(const glm::ivec3 &position, Block::Type type) const
{
  if (!is_valid_block_position(position))
  {
    return false;
  }

  const auto &other_block_type =
      blocks_[position.x][position.z][position.y].type();
  if (type == Block::Type::Water)
  {
    return other_block_type != Block::Type::Air;
  }

  return other_block_type != Block::Type::Air &&
         other_block_type != Block::Type::Water;
}

bool Chunk::is_block(const glm::ivec3 &position,
                     const World      &world,
                     Block::Type       type) const
{
  if (is_valid_block_position(position))
  {
    return is_block(position, type);
  }
  const auto world_position = block_position_to_world_position(position);
  return world.is_block(world_position, type);
}

void Chunk::generate_mesh_data(const World            &world,
                               std::vector<glm::vec3> &positions,
                               std::vector<glm::vec3> &normals,
                               std::vector<glm::vec2> &tex_coords,
                               std::vector<unsigned>  &indices)
{
  int current_index = 0;
  for (int x = 0; x < blocks_.size(); ++x)
  {
    for (int z = 0; z < blocks_[x].size(); ++z)
    {
      for (int y = 0; y < blocks_[x][z].size(); ++y)
      {
        const auto block_type = blocks_[x][z][y].type();
        const auto block_height =
            block_type == Block::Type::Water ? 0.9f : 1.0f;
        if (block_type == Block::Type::Air)
        {
          continue;
        }
        // Front
        if (!is_block(glm::ivec3{x, y, z + 1}, world, block_type))
        {
          positions.emplace_back(x + 0.0f, y + block_height, z + 1.0f);
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + block_height, z + 1.0f);

          normals.emplace_back(0.0f, 0.0f, 1.0f);
          normals.emplace_back(0.0f, 0.0f, 1.0f);
          normals.emplace_back(0.0f, 0.0f, 1.0f);
          normals.emplace_back(0.0f, 0.0f, 1.0f);

          const auto coords =
              block_type_to_texture_coords(blocks_[x][z][y].type(),
                                           BlockSide::Front,
                                           world);
          tex_coords.emplace_back(coords.bottom_left);
          tex_coords.emplace_back(coords.top_left);
          tex_coords.emplace_back(coords.top_right);
          tex_coords.emplace_back(coords.bottom_right);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Back
        if (!is_block(glm::ivec3{x, y, z - 1}, world, block_type))
        {
          positions.emplace_back(x + 0.0f, y + block_height, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + block_height, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);

          normals.emplace_back(0.0f, 0.0f, -1.0f);
          normals.emplace_back(0.0f, 0.0f, -1.0f);
          normals.emplace_back(0.0f, 0.0f, -1.0f);
          normals.emplace_back(0.0f, 0.0f, -1.0f);

          const auto coords =
              block_type_to_texture_coords(blocks_[x][z][y].type(),
                                           BlockSide::Back,
                                           world);
          tex_coords.emplace_back(coords.bottom_right);
          tex_coords.emplace_back(coords.bottom_left);
          tex_coords.emplace_back(coords.top_left);
          tex_coords.emplace_back(coords.top_right);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Top
        if (!is_block(glm::ivec3{x, y + 1, z}, world, block_type))
        {
          positions.emplace_back(x + 0.0f, y + block_height, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + block_height, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + block_height, z + 0.0f);
          positions.emplace_back(x + 0.0f, y + block_height, z + 0.0f);

          normals.emplace_back(0.0f, 1.0f, 0.0f);
          normals.emplace_back(0.0f, 1.0f, 0.0f);
          normals.emplace_back(0.0f, 1.0f, 0.0f);
          normals.emplace_back(0.0f, 1.0f, 0.0f);

          const auto coords =
              block_type_to_texture_coords(blocks_[x][z][y].type(),
                                           BlockSide::Top,
                                           world);
          tex_coords.emplace_back(coords.top_left);
          tex_coords.emplace_back(coords.top_right);
          tex_coords.emplace_back(coords.bottom_right);
          tex_coords.emplace_back(coords.bottom_left);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Bottom
        if (!is_block(glm::ivec3{x, y - 1, z}, world, block_type))
        {
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);

          normals.emplace_back(0.0f, -1.0f, 0.0f);
          normals.emplace_back(0.0f, -1.0f, 0.0f);
          normals.emplace_back(0.0f, -1.0f, 0.0f);
          normals.emplace_back(0.0f, -1.0f, 0.0f);

          const auto coords =
              block_type_to_texture_coords(blocks_[x][z][y].type(),
                                           BlockSide::Bottom,
                                           world);
          tex_coords.emplace_back(coords.bottom_left);
          tex_coords.emplace_back(coords.top_left);
          tex_coords.emplace_back(coords.top_right);
          tex_coords.emplace_back(coords.bottom_right);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Left
        if (!is_block(glm::ivec3{x - 1, y, z}, world, block_type))
        {
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 0.0f, y + block_height, z + 1.0f);
          positions.emplace_back(x + 0.0f, y + block_height, z + 0.0f);
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);

          normals.emplace_back(-1.0f, 0.0f, 0.0f);
          normals.emplace_back(-1.0f, 0.0f, 0.0f);
          normals.emplace_back(-1.0f, 0.0f, 0.0f);
          normals.emplace_back(-1.0f, 0.0f, 0.0f);

          const auto coords =
              block_type_to_texture_coords(blocks_[x][z][y].type(),
                                           BlockSide::Left,
                                           world);
          tex_coords.emplace_back(coords.top_right);
          tex_coords.emplace_back(coords.bottom_right);
          tex_coords.emplace_back(coords.bottom_left);
          tex_coords.emplace_back(coords.top_left);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Right
        if (!is_block(glm::ivec3{x + 1, y, z}, world, block_type))
        {
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + block_height, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + block_height, z + 1.0f);

          normals.emplace_back(1.0f, 0.0f, 0.0f);
          normals.emplace_back(1.0f, 0.0f, 0.0f);
          normals.emplace_back(1.0f, 0.0f, 0.0f);
          normals.emplace_back(1.0f, 0.0f, 0.0f);

          const auto coords =
              block_type_to_texture_coords(blocks_[x][z][y].type(),
                                           BlockSide::Right,
                                           world);
          tex_coords.emplace_back(coords.top_left);
          tex_coords.emplace_back(coords.top_right);
          tex_coords.emplace_back(coords.bottom_right);
          tex_coords.emplace_back(coords.bottom_left);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }
      }
    }
  }
}

void Chunk::fill_mesh_data(const World &world)
{
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> tex_coords;
  std::vector<unsigned>  indices;
  generate_mesh_data(world, positions, normals, tex_coords, indices);
  send_mesh_data_to_gpu(positions, normals, tex_coords, indices);
}

void Chunk::send_mesh_data_to_gpu(const std::vector<glm::vec3> &positions,
                                  const std::vector<glm::vec3> &normals,
                                  const std::vector<glm::vec2> &tex_coords,
                                  const std::vector<unsigned>  &indices)
{
  {
    GlVertexBufferLayout vec3_layout;
    vec3_layout.push_float(3);
    vertex_buffer_positions_->set_data(positions, vec3_layout);
    vertex_buffer_normals_->set_data(normals, vec3_layout);
  }

  {
    GlVertexBufferLayout vec2_layout;
    vec2_layout.push_float(2);
    vertex_buffer_tex_coords_->set_data(tex_coords, vec2_layout);
  }

  index_buffer_->set_data(indices);
}

void Chunk::regenerate_mesh(const World &world) { fill_mesh_data(world); }

void Chunk::generate_mesh(const World &world)
{
  if (is_mesh_generated_)
  {
    return;
  }
  is_mesh_generated_ = true;

  if (!vertex_buffer_positions_)
  {
    vertex_buffer_positions_ = std::make_unique<GlVertexBuffer>();
  }
  if (!vertex_buffer_normals_)
  {
    vertex_buffer_normals_ = std::make_unique<GlVertexBuffer>();
  }
  if (!vertex_buffer_tex_coords_)
  {
    vertex_buffer_tex_coords_ = std::make_unique<GlVertexBuffer>();
  }
  if (!index_buffer_)
  {
    index_buffer_ = std::make_unique<GlIndexBuffer>();
  }

  vertex_buffers_.push_back(vertex_buffer_positions_.get());
  vertex_buffers_.push_back(vertex_buffer_normals_.get());
  vertex_buffers_.push_back(vertex_buffer_tex_coords_.get());

  fill_mesh_data(world);
}

void Chunk::draw(GlShader &shader)
{
  assert(vertex_buffer_positions_ != nullptr &&
         vertex_buffer_normals_ != nullptr &&
         vertex_buffer_tex_coords_ != nullptr && index_buffer_ != nullptr);

  shader.draw(vertex_buffers_, *index_buffer_);
}

glm::ivec3 Chunk::position() const { return position_; }

Block::Type Chunk::block_type(const glm::ivec3 &position) const
{
  assert(
      is_valid_block_position(glm::ivec3{position.x, position.y, position.z}));

  return blocks_[position.x][position.z][position.y].type();
}

bool Chunk::is_valid_block_position(const glm::ivec3 &position) const
{
  const auto x = position.x;
  const auto y = position.y;
  const auto z = position.z;

  return ((0 <= x && x < blocks_.size()) && (0 <= z && z < blocks_[x].size()) &&
          (0 <= y && y < blocks_[x][z].size()));
}

bool Chunk::remove_block(World &world, const glm::ivec3 &position)
{
  std::cout << "Try to remove block: " << position << std::endl;
  if (!is_valid_block_position(position))
  {
    std::cout << "Not a valid block" << std::endl;
    return false;
  }

  auto &block = blocks_[position.x][position.z][position.y];

  if (block.type() == Block::Type::Air || block.type() == Block::Type::Water)
  {
    std::cout << "Can not remove air or water block" << std::endl;
    return false;
  }

  std::cout << "Remove block" << std::endl;
  block.set_type(Block::Type::Air);
  regenerate_mesh(world);
  regenerate_chunks_if_border_block(world, position);

  return true;
}

Block &Chunk::block(const glm::ivec3 &position)
{
  assert(is_valid_block_position(position));
  return blocks_[position.x][position.z][position.y];
}

void Chunk::regenerate_chunks_if_border_block(World            &world,
                                              const glm::ivec3 &position)
{
  // TODO: Optimize check if neighbour block is solid
  if (position.x == 0)
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x - 1, position_.y, position_.z});
  }
  else if (position.x == blocks_.size() - 1)
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x + 1, position_.y, position_.z});
  }

  if (position.z == 0)
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x, position_.y, position_.z - 1});
  }
  else if (position.z == blocks_[position.x].size() - 1)
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x, position_.y, position_.z + 1});
  }

  if (position.y == 0)
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x, position_.y - 1, position_.z});
  }
  else if (position.y == blocks_[position.x][position.z].size() - 1)
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x, position_.y + 1, position_.z});
  }
}

glm::ivec3
Chunk::block_position_to_world_position(const glm::ivec3 &block_position) const
{
  // // FIXME: This won't work for value that are in abs bigger than
  // Chunk::width
  // // We need to account for out of bounds
  const auto chunk_position      = position_;
  const auto real_block_position = block_position;

  const auto x = real_block_position.x + chunk_position.x * Chunk::width();
  const auto z = real_block_position.z + chunk_position.z * Chunk::width();
  const auto y = real_block_position.y + chunk_position.y * Chunk::height();

  return {x, y, z};
}

bool Chunk::place_block(World            &world,
                        const glm::ivec3 &position,
                        Block::Type       block_type)
{
  std::cout << "Try to place block: " << position << std::endl;
  if (!is_valid_block_position(position))
  {
    std::cout << "Not a valid block" << std::endl;
    return false;
  }

  auto &block = blocks_[position.x][position.z][position.y];
  block.set_type(block_type);

  regenerate_mesh(world);
  regenerate_chunks_if_border_block(world, position);

  return true;
}
