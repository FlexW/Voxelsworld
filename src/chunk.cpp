#include "chunk.hpp"
#include "application.hpp"
#include "block.hpp"
#include "gl/gl_index_buffer.hpp"
#include "gl/gl_vertex_buffer.hpp"
#include "glm/gtc/noise.hpp"
#include "math.hpp"
#include "texture_atlas.hpp"
#include "world.hpp"

#include <cassert>
#include <limits>
#include <memory>
#include <random>

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

  c1_              = config.config_value_float("Chunk", "c1", 1.0f);
  c2_              = config.config_value_float("Chunk", "c2", 0.7f);
  c3_              = config.config_value_float("Chunk", "c3", 0.008f);
  div_             = config.config_value_float("Chunk", "div", 1.0f);
  frequency1_      = config.config_value_float("Chunk", "frequency1", 0.0003f);
  frequency2_      = config.config_value_float("Chunk", "frequency2", 0.008f);
  frequency3_      = config.config_value_float("Chunk", "frequency3", 0.1f);
  e_               = config.config_value_float("Chunk", "e", 11.3);
  fudge_factor_    = config.config_value_float("Chunk", "fudge_factor", 1.1);
  water_level_     = config.config_value_float("Chunk", "water_level", 5.0);
  terraces_        = config.config_value_float("Chunk", "terraces", 180.0);
  tree_density_    = config.config_value_int("Chunk", "tree_density", 6);
  min_tree_height_ = config.config_value_int("Chunk", "min_tree_height", 5);
  max_tree_height_ = config.config_value_int("Chunk", "max_tree_height", 11);
  min_leaves_radius_ = config.config_value_int("Chunk", "min_leaves_radius", 3);
  max_leaves_radius_ = config.config_value_int("Chunk", "max_leaves_radius", 5);
  leave_density_     = config.config_value_int("Chunk", "leaves_density", 2);

  blocks_.resize(width());
  for (std::size_t x = 0; x < blocks_.size(); ++x)
  {
    blocks_[x].resize(width());
    for (std::size_t z = 0; z < blocks_[x].size(); ++z)
    {
      blocks_[x][z].resize(height());
    }
  }
}

[[nodiscard]] bool Chunk::is_generated() const { return is_generated_; }

void Chunk::generate(const glm::vec3 &position, const World & /*world*/)
{
  if (is_generated_)
  {
    return;
  }

  position_ = glm::ivec3(static_cast<int>(position.x),
                         static_cast<int>(position.y),
                         static_cast<int>(position.z));

  // Generate blue noise
  std::vector<std::vector<float>> blue_noise;
  blue_noise.resize(blocks_.size());
  for (std::size_t x = 0; x < blocks_.size(); ++x)
  {
    blue_noise[x].resize(blocks_[x].size());
    for (std::size_t z = 0; z < blocks_.size(); ++z)
    {
      const auto world_position =
          block_position_to_world_position(glm::ivec3{x, 0, z});

      auto position_x = world_position.x + 0.5f;
      auto position_z = world_position.z + 0.5f;

      blue_noise[x][z] =
          glm::simplex(glm::vec2{50.0f * position_x, 50.0f * position_z});
    }
  }

  for (std::size_t x = 0; x < blocks_.size(); ++x)
  {
    for (std::size_t z = 0; z < blocks_[x].size(); ++z)
    {
      const auto world_position =
          block_position_to_world_position(glm::ivec3{x, 0, z});

      auto position_x = world_position.x + 0.5f;
      auto position_z = world_position.z + 0.5f;

      // Calculate if a tree needs to be placed
      double max = 0;
      // there are more efficient algorithms than this
      for (int yn = x - tree_density_;
           yn <= static_cast<int>(x) + tree_density_;
           yn++)
      {
        for (int xn = z - tree_density_;
             xn <= static_cast<int>(z) + tree_density_;
             xn++)
        {
          if (0 <= yn && yn < width() && 0 <= xn && xn < width())
          {
            const auto e = blue_noise[yn][xn];
            if (e > max)
            {
              max = e;
            }
          }
        }
      }
      bool place_tree = false;
      if (blue_noise[x][z] == max)
      {
        place_tree = true;
      }

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
      std::mt19937                       rng(94);
      std::uniform_int_distribution<int> tree_height_gen(min_tree_height_,
                                                         max_tree_height_);
      std::uniform_int_distribution<int> leave_radius_gen(min_leaves_radius_,
                                                          max_leaves_radius_);
      std::uniform_int_distribution<int> leave_density_gen(0, leave_density_);

      for (int y = 0; y <= height || y <= water_level_; ++y)
      {
        assert(0 <= y && y < blocks_[x][z].size());
        auto &block = blocks_[x][z][y];
        if (y == height)
        {
          if (y < water_level_)
          {
            block.set_type(Block::Type::Dirt);
          }
          else
          {
            block.set_type(Block::Type::Grass);

            // Check if we need to place a tree
            if (place_tree)
            {
              // Place tree
              const auto tree_height = tree_height_gen(rng);
              for (int i = y + 1; i < static_cast<int>(y) + tree_height &&
                                  i < static_cast<int>(blocks_[x][z].size());
                   ++i)
              {
                blocks_[x][z][i].set_type(Block::Type::Oak);
              }

              // Place leaves
              const auto leave_radius = leave_radius_gen(rng);
              for (int leave_x = -leave_radius; leave_x < leave_radius + 1;
                   ++leave_x)
              {
                for (int leave_z = -leave_radius; leave_z < leave_radius + 1;
                     ++leave_z)
                {
                  for (int leave_y = -leave_radius; leave_y < leave_radius + 1;
                       ++leave_y)
                  {
                    const int real_leave_x{static_cast<int>(x + leave_x)};
                    const int real_leave_y{tree_height + y + leave_y};
                    const int real_leave_z{static_cast<int>(z + leave_z)};

                    if (real_leave_x < 0 || real_leave_x >= Chunk::width() ||
                        real_leave_z < 0 || real_leave_z >= Chunk::width() ||
                        real_leave_y < 0 || real_leave_y >= Chunk::height())
                    {
                      continue;
                    }
                    if (leave_density_gen(rng) != leave_density_)
                    {
                      continue;
                    }
                    if (blocks_[real_leave_x][real_leave_z][real_leave_y]
                            .type() == Block::Type::Air)
                    {
                      blocks_[real_leave_x][real_leave_z][real_leave_y]
                          .set_type(Block::Type::OakLeaves);
                    }
                  }
                }
              }
            }
          }
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
                               std::vector<glm::vec3> &block_positions,
                               std::vector<glm::vec3> &block_normals,
                               std::vector<glm::vec2> &block_tex_coords,
                               std::vector<int>       &block_tex_indices,
                               std::vector<unsigned>  &block_indices,
                               std::vector<glm::vec3> &water_positions,
                               std::vector<glm::vec3> &water_normals,
                               std::vector<glm::vec2> &water_tex_coords,
                               std::vector<int>       &water_tex_indices,
                               std::vector<unsigned>  &water_indices)
{
  int current_index_block = 0;
  int current_index_water = 0;
  for (std::size_t x = 0; x < blocks_.size(); ++x)
  {
    for (std::size_t z = 0; z < blocks_[x].size(); ++z)
    {
      for (std::size_t y = 0; y < blocks_[x][z].size(); ++y)
      {
        const auto block_type = blocks_[x][z][y].type();
        const auto block_height =
            block_type == Block::Type::Water ? 0.9f : 1.0f;

        auto positions     = &block_positions;
        auto normals       = &block_normals;
        auto tex_coords    = &block_tex_coords;
        auto tex_indices   = &block_tex_indices;
        auto indices       = &block_indices;
        auto current_index = &current_index_block;

        if (block_type == Block::Type::Air)
        {
          continue;
        }
        else if (block_type == Block::Type::Water)
        {
          positions     = &water_positions;
          normals       = &water_normals;
          tex_coords    = &water_tex_coords;
          tex_indices   = &water_tex_indices;
          indices       = &water_indices;
          current_index = &current_index_water;
        }

        // Front
        if (!is_block(glm::ivec3{x, y, z + 1}, world, block_type))
        {
          positions->emplace_back(x + 0.0f, y + block_height, z + 1.0f);
          positions->emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions->emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);
          positions->emplace_back(x + 1.0f, y + block_height, z + 1.0f);

          normals->emplace_back(0.0f, 0.0f, 1.0f);
          normals->emplace_back(0.0f, 0.0f, 1.0f);
          normals->emplace_back(0.0f, 0.0f, 1.0f);
          normals->emplace_back(0.0f, 0.0f, 1.0f);

          tex_coords->emplace_back(0.0f, 1.0f);
          tex_coords->emplace_back(0.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 1.0f);

          const auto tex_index =
              world.block_texture_index(block_type, Block::Side::Front);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);

          indices->push_back(*current_index + 0);
          indices->push_back(*current_index + 1);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 3);
          indices->push_back(*current_index + 0);
          *current_index += 4;
        }

        // Back
        if (!is_block(glm::ivec3{x, y, z - 1}, world, block_type))
        {
          positions->emplace_back(x + 0.0f, y + block_height, z + 0.0f);
          positions->emplace_back(x + 1.0f, y + block_height, z + 0.0f);
          positions->emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions->emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);

          normals->emplace_back(0.0f, 0.0f, -1.0f);
          normals->emplace_back(0.0f, 0.0f, -1.0f);
          normals->emplace_back(0.0f, 0.0f, -1.0f);
          normals->emplace_back(0.0f, 0.0f, -1.0f);

          tex_coords->emplace_back(1.0f, 1.0f);
          tex_coords->emplace_back(0.0f, 1.0f);
          tex_coords->emplace_back(0.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 0.0f);

          const auto tex_index =
              world.block_texture_index(block_type, Block::Side::Back);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);

          indices->push_back(*current_index + 0);
          indices->push_back(*current_index + 1);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 3);
          indices->push_back(*current_index + 0);
          *current_index += 4;
        }

        // Top
        if (!is_block(glm::ivec3{x, y + 1, z}, world, block_type))
        {
          positions->emplace_back(x + 0.0f, y + block_height, z + 1.0f);
          positions->emplace_back(x + 1.0f, y + block_height, z + 1.0f);
          positions->emplace_back(x + 1.0f, y + block_height, z + 0.0f);
          positions->emplace_back(x + 0.0f, y + block_height, z + 0.0f);

          normals->emplace_back(0.0f, 1.0f, 0.0f);
          normals->emplace_back(0.0f, 1.0f, 0.0f);
          normals->emplace_back(0.0f, 1.0f, 0.0f);
          normals->emplace_back(0.0f, 1.0f, 0.0f);

          tex_coords->emplace_back(0.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 1.0f);
          tex_coords->emplace_back(0.0f, 1.0f);

          const auto tex_index =
              world.block_texture_index(block_type, Block::Side::Top);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);

          indices->push_back(*current_index + 0);
          indices->push_back(*current_index + 1);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 3);
          indices->push_back(*current_index + 0);
          *current_index += 4;
        }

        // Bottom
        if (!is_block(glm::ivec3{x, y - 1, z}, world, block_type))
        {
          positions->emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions->emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);
          positions->emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions->emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);

          normals->emplace_back(0.0f, -1.0f, 0.0f);
          normals->emplace_back(0.0f, -1.0f, 0.0f);
          normals->emplace_back(0.0f, -1.0f, 0.0f);
          normals->emplace_back(0.0f, -1.0f, 0.0f);

          tex_coords->emplace_back(0.0f, 1.0f);
          tex_coords->emplace_back(0.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 1.0f);

          const auto tex_index =
              world.block_texture_index(block_type, Block::Side::Bottom);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);

          indices->push_back(*current_index + 0);
          indices->push_back(*current_index + 1);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 3);
          indices->push_back(*current_index + 0);
          *current_index += 4;
        }

        // Left
        if (!is_block(glm::ivec3{x - 1, y, z}, world, block_type))
        {
          positions->emplace_back(x + 0.0f, y + 0.0f, z + 1.0f);
          positions->emplace_back(x + 0.0f, y + block_height, z + 1.0f);
          positions->emplace_back(x + 0.0f, y + block_height, z + 0.0f);
          positions->emplace_back(x + 0.0f, y + 0.0f, z + 0.0f);

          normals->emplace_back(-1.0f, 0.0f, 0.0f);
          normals->emplace_back(-1.0f, 0.0f, 0.0f);
          normals->emplace_back(-1.0f, 0.0f, 0.0f);
          normals->emplace_back(-1.0f, 0.0f, 0.0f);

          tex_coords->emplace_back(1.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 1.0f);
          tex_coords->emplace_back(0.0f, 1.0f);
          tex_coords->emplace_back(0.0f, 0.0f);

          const auto tex_index =
              world.block_texture_index(block_type, Block::Side::Left);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);

          indices->push_back(*current_index + 0);
          indices->push_back(*current_index + 1);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 3);
          indices->push_back(*current_index + 0);
          *current_index += 4;
        }

        // Right
        if (!is_block(glm::ivec3{x + 1, y, z}, world, block_type))
        {
          positions->emplace_back(x + 1.0f, y + 0.0f, z + 1.0f);
          positions->emplace_back(x + 1.0f, y + 0.0f, z + 0.0f);
          positions->emplace_back(x + 1.0f, y + block_height, z + 0.0f);
          positions->emplace_back(x + 1.0f, y + block_height, z + 1.0f);

          normals->emplace_back(1.0f, 0.0f, 0.0f);
          normals->emplace_back(1.0f, 0.0f, 0.0f);
          normals->emplace_back(1.0f, 0.0f, 0.0f);
          normals->emplace_back(1.0f, 0.0f, 0.0f);

          tex_coords->emplace_back(0.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 0.0f);
          tex_coords->emplace_back(1.0f, 1.0f);
          tex_coords->emplace_back(0.0f, 1.0f);

          const auto tex_index =
              world.block_texture_index(block_type, Block::Side::Right);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);
          tex_indices->push_back(tex_index);

          indices->push_back(*current_index + 0);
          indices->push_back(*current_index + 1);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 2);
          indices->push_back(*current_index + 3);
          indices->push_back(*current_index + 0);
          *current_index += 4;
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
  std::vector<int>       tex_indices;
  std::vector<unsigned>  indices;

  std::vector<glm::vec3> water_positions;
  std::vector<glm::vec3> water_normals;
  std::vector<glm::vec2> water_tex_coords;
  std::vector<int>       water_tex_indices;
  std::vector<unsigned>  water_indices;

  generate_mesh_data(world,
                     positions,
                     normals,
                     tex_coords,
                     tex_indices,
                     indices,
                     water_positions,
                     water_normals,
                     water_tex_coords,
                     water_tex_indices,
                     water_indices);

  send_mesh_data_to_gpu(positions,
                        normals,
                        tex_coords,
                        tex_indices,
                        indices,
                        water_positions,
                        water_normals,
                        water_tex_coords,
                        water_tex_indices,
                        water_indices);
}

void Chunk::send_mesh_data_to_gpu(
    const std::vector<glm::vec3> &block_positions,
    const std::vector<glm::vec3> &block_normals,
    const std::vector<glm::vec2> &block_tex_coords,
    const std::vector<int>       &block_tex_indices,
    const std::vector<unsigned>  &block_indices,
    const std::vector<glm::vec3> &water_positions,
    const std::vector<glm::vec3> &water_normals,
    const std::vector<glm::vec2> &water_tex_coords,
    const std::vector<int>       &water_tex_indices,
    const std::vector<unsigned>  &water_indices)
{
  {
    GlVertexBufferLayout vec3_layout;
    vec3_layout.push_float(3);
    vertex_buffer_positions_->set_data(block_positions, vec3_layout);
    vertex_buffer_normals_->set_data(block_normals, vec3_layout);
    vertex_buffer_water_positions_->set_data(water_positions, vec3_layout);
    vertex_buffer_water_normals_->set_data(water_normals, vec3_layout);
  }

  {
    GlVertexBufferLayout int_layout;
    int_layout.push_int(1);
    vertex_buffer_tex_indices_->set_data(block_tex_indices, int_layout);
    vertex_buffer_water_tex_indices_->set_data(water_tex_indices, int_layout);
  }

  {
    GlVertexBufferLayout vec2_layout;
    vec2_layout.push_float(2);
    vertex_buffer_tex_coords_->set_data(block_tex_coords, vec2_layout);
    vertex_buffer_water_tex_coords_->set_data(water_tex_coords, vec2_layout);
  }

  index_buffer_->set_data(block_indices);
  water_index_buffer_->set_data(water_indices);
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
  if (!vertex_buffer_tex_indices_)
  {
    vertex_buffer_tex_indices_ = std::make_unique<GlVertexBuffer>();
  }
  if (!index_buffer_)
  {
    index_buffer_ = std::make_unique<GlIndexBuffer>();
  }

  vertex_buffers_.push_back(vertex_buffer_positions_.get());
  vertex_buffers_.push_back(vertex_buffer_normals_.get());
  vertex_buffers_.push_back(vertex_buffer_tex_coords_.get());
  vertex_buffers_.push_back(vertex_buffer_tex_indices_.get());

  if (!vertex_buffer_water_positions_)
  {
    vertex_buffer_water_positions_ = std::make_unique<GlVertexBuffer>();
  }
  if (!vertex_buffer_water_normals_)
  {
    vertex_buffer_water_normals_ = std::make_unique<GlVertexBuffer>();
  }
  if (!vertex_buffer_water_tex_coords_)
  {
    vertex_buffer_water_tex_coords_ = std::make_unique<GlVertexBuffer>();
  }
  if (!vertex_buffer_water_tex_indices_)
  {
    vertex_buffer_water_tex_indices_ = std::make_unique<GlVertexBuffer>();
  }
  if (!water_index_buffer_)
  {
    water_index_buffer_ = std::make_unique<GlIndexBuffer>();
  }

  water_vertex_buffers_.push_back(vertex_buffer_water_positions_.get());
  water_vertex_buffers_.push_back(vertex_buffer_water_normals_.get());
  water_vertex_buffers_.push_back(vertex_buffer_water_tex_coords_.get());
  water_vertex_buffers_.push_back(vertex_buffer_water_tex_indices_.get());

  fill_mesh_data(world);
}

void Chunk::draw(GlShader &shader)
{
  assert(vertex_buffer_positions_ != nullptr &&
         vertex_buffer_normals_ != nullptr &&
         vertex_buffer_tex_coords_ != nullptr && index_buffer_ != nullptr);
  if (index_buffer_->count() == 0)
  {
    return;
  }

  shader.draw(vertex_buffers_, *index_buffer_);
}

void Chunk::draw_water(GlShader &shader)
{
  if (water_index_buffer_->count() == 0)
  {
    return;
  }
  shader.draw(water_vertex_buffers_, *water_index_buffer_);
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

  return ((0 <= x && x < static_cast<int>(blocks_.size())) &&
          (0 <= z && z < static_cast<int>(blocks_[x].size())) &&
          (0 <= y && y < static_cast<int>(blocks_[x][z].size())));
}

bool Chunk::remove_block(World &world, const glm::ivec3 &position)
{
  if (!is_valid_block_position(position))
  {
    return false;
  }

  auto &block = blocks_[position.x][position.z][position.y];

  if (block.type() == Block::Type::Air || block.type() == Block::Type::Water)
  {
    return false;
  }

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
  else if (position.x == static_cast<int>(blocks_.size() - 1))
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x + 1, position_.y, position_.z});
  }

  if (position.z == 0)
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x, position_.y, position_.z - 1});
  }
  else if (position.z == static_cast<int>(blocks_[position.x].size() - 1))
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x, position_.y, position_.z + 1});
  }

  if (position.y == 0)
  {
    world.regenerate_chunk(
        glm::ivec3{position_.x, position_.y - 1, position_.z});
  }
  else if (position.y ==
           static_cast<int>(blocks_[position.x][position.z].size() - 1))
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
  if (!is_valid_block_position(position))
  {
    return false;
  }

  auto &block = blocks_[position.x][position.z][position.y];
  block.set_type(block_type);

  regenerate_mesh(world);
  regenerate_chunks_if_border_block(world, position);

  return true;
}
