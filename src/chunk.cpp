#include "chunk.hpp"
#include "block.hpp"
#include "gl/gl_index_buffer.hpp"
#include "gl/gl_vertex_buffer.hpp"
#include "math.hpp"
#include "world.hpp"

#include <limits>
#include <memory>

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

  // blocks_[0][0][0].set_type(Block::Type::Grass);
  // blocks_[1][0][0].set_type(Block::Type::Grass);

  for (int x = 0; x < blocks_.size(); ++x)
  {
    for (int z = 0; z < blocks_[x].size(); ++z)
    {
      for (int y = 0; y < blocks_[x][z].size(); ++y)
      {
        auto &block = blocks_[x][z][y];
        if (y == 0 /*|| (y == 1 && x % 2 == 0 && z % 2 == 0)*/)
        {
          block.set_type(Block::Type::Grass);
        }
      }
    }
  }

  // NOISE ---------------

  // static constexpr auto frequency = 0.005f;

  // for (int x = 0; x < blocks_.size(); ++x)
  // {
  //   for (int z = 0; z < blocks_[x].size(); ++z)
  //   {
  //     auto position_x = x + position.x + 0.5f;
  //     auto position_z = z + position.z + 0.5f;

  //     const auto n = glm::simplex(
  //         glm::vec2(position_x * frequency, position_z * frequency));
  //     const auto height = (n + 1.0f) * Chunk::height;

  //     std::cout << "x: " << position_x << " z: " << position_z << " y: " << n
  //               << std::endl;

  //     for (int y = 0; y <= height; ++y)
  //     {
  //       auto &block = blocks_[x][z][y];
  //       block.set_type(Block::Type::Grass);
  //     }
  //   }
  // }

  is_generated_ = true;
}

[[nodiscard]] bool Chunk::is_mesh_generated() const
{
  return is_mesh_generated_;
}

bool Chunk::is_block(const glm::ivec3 &position) const
{
  if (!is_valid_block_position(position))
  {
    return false;
  }

  return blocks_[position.x][position.z][position.y].type() != Block::Type::Air;
}

bool Chunk::is_block(const glm::ivec3 &position, const World &world) const
{
  if (is_valid_block_position(position))
  {
    return is_block(position);
  }
  const auto world_position = block_position_to_world_position(position);
  return world.is_block(world_position);
}

void Chunk::fill_mesh_data(const World &world)
{
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> tex_coords;
  std::vector<unsigned>  indices;

  int current_index = 0;
  for (int x = 0; x < blocks_.size(); ++x)
  {
    for (int z = 0; z < blocks_[x].size(); ++z)
    {
      for (int y = 0; y < blocks_[x][z].size(); ++y)
      {
        if (blocks_[x][z][y].type() == Block::Type::Air)
        {
          continue;
        }
        // Front
        if (!is_block(glm::ivec3{x, y, z + 1}, world))
        {
          positions.emplace_back(x + 0.0f, y + 1.0f, z + 1.0f);
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + 1.0f, z + 1.0f);

          normals.emplace_back(0.0f, 0.0f, 1.0f);
          normals.emplace_back(0.0f, 0.0f, 1.0f);
          normals.emplace_back(0.0f, 0.0f, 1.0f);
          normals.emplace_back(0.0f, 0.0f, 1.0f);

          tex_coords.emplace_back(0.0f, 1.0f);
          tex_coords.emplace_back(0.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 1.0f);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Back
        if (!is_block(glm::ivec3{x, y, z - 1}, world))
        {
          positions.emplace_back(x + 0.0f, y + 1.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 1.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);

          normals.emplace_back(0.0f, 0.0f, -1.0f);
          normals.emplace_back(0.0f, 0.0f, -1.0f);
          normals.emplace_back(0.0f, 0.0f, -1.0f);
          normals.emplace_back(0.0f, 0.0f, -1.0f);

          tex_coords.emplace_back(1.0f, 1.0f);
          tex_coords.emplace_back(0.0f, 1.0f);
          tex_coords.emplace_back(0.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 0.0f);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Top
        if (!is_block(glm::ivec3{x, y + 1, z}, world))
        {
          positions.emplace_back(x + 0.0f, y + 1.0f, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + 1.0f, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + 1.0f, z + 0.0f);
          positions.emplace_back(x + 0.0f, y + 1.0f, z + 0.0f);

          normals.emplace_back(0.0f, 1.0f, 0.0f);
          normals.emplace_back(0.0f, 1.0f, 0.0f);
          normals.emplace_back(0.0f, 1.0f, 0.0f);
          normals.emplace_back(0.0f, 1.0f, 0.0f);

          tex_coords.emplace_back(0.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 1.0f);
          tex_coords.emplace_back(0.0f, 1.0f);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Bottom
        if (!is_block(glm::ivec3{x, y - 1, z}, world))
        {
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);

          normals.emplace_back(0.0f, -1.0f, 0.0f);
          normals.emplace_back(0.0f, -1.0f, 0.0f);
          normals.emplace_back(0.0f, -1.0f, 0.0f);
          normals.emplace_back(0.0f, -1.0f, 0.0f);

          tex_coords.emplace_back(0.0f, 1.0f);
          tex_coords.emplace_back(0.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 1.0f);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Left
        if (!is_block(glm::ivec3{x - 1, y, z}, world))
        {
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 0.0f, y + 1.0f, z + 1.0f);
          positions.emplace_back(x + 0.0f, y + 1.0f, z + 0.0f);
          positions.emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);

          normals.emplace_back(-1.0f, 0.0f, 0.0f);
          normals.emplace_back(-1.0f, 0.0f, 0.0f);
          normals.emplace_back(-1.0f, 0.0f, 0.0f);
          normals.emplace_back(-1.0f, 0.0f, 0.0f);

          tex_coords.emplace_back(1.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 1.0f);
          tex_coords.emplace_back(0.0f, 1.0f);
          tex_coords.emplace_back(0.0f, 0.0f);

          indices.push_back(current_index + 0);
          indices.push_back(current_index + 1);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 2);
          indices.push_back(current_index + 3);
          indices.push_back(current_index + 0);
          current_index += 4;
        }

        // Right
        if (!is_block(glm::ivec3{x + 1, y, z}, world))
        {
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);
          positions.emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 1.0f, z + 0.0f);
          positions.emplace_back(x + 1.0f, y + 1.0f, z + 1.0f);

          normals.emplace_back(1.0f, 0.0f, 0.0f);
          normals.emplace_back(1.0f, 0.0f, 0.0f);
          normals.emplace_back(1.0f, 0.0f, 0.0f);
          normals.emplace_back(1.0f, 0.0f, 0.0f);

          tex_coords.emplace_back(0.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 0.0f);
          tex_coords.emplace_back(1.0f, 1.0f);
          tex_coords.emplace_back(0.0f, 1.0f);

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

bool Chunk::remove_block(const World &world, const glm::ivec3 &position)
{
  std::cout << "Try to remove block: " << position << std::endl;
  if (!is_valid_block_position(position))
  {
    std::cout << "Not a valid block" << std::endl;
    return false;
  }

  auto &block = blocks_[position.x][position.z][position.y];

  if (block.type() == Block::Type::Air)
  {
    std::cout << "Can not remove air block" << std::endl;
    return false;
  }

  std::cout << "Remove block" << std::endl;
  block.set_type(Block::Type::Air);
  regenerate_mesh(world);

  // TODO: Check if border block
  // If border block check if neigbour block is solid
  // If solid regenerate mesh of neigbour chunk

  return true;
}

Block &Chunk::block(const glm::ivec3 &position)
{
  assert(is_valid_block_position(position));
  return blocks_[position.x][position.z][position.y];
}

glm::ivec3
Chunk::block_position_to_world_position(const glm::ivec3 &block_position) const
{
  // // FIXME: This won't work for value that are in abs bigger than
  // Chunk::width
  // // We need to account for out of bounds
  const auto chunk_position      = position_;
  const auto real_block_position = block_position;

  const auto x = real_block_position.x + chunk_position.x * Chunk::width;
  const auto z = real_block_position.z + chunk_position.z * Chunk::width;
  const auto y = real_block_position.y + chunk_position.y * Chunk::height;

  return {x, y, z};
}

bool Chunk::place_block(const World      &world,
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

  // TODO: Check if border block
  // If border block check if neigbour block is solid
  // If solid regenerate mesh of neigbour chunk

  return true;
}
