#include "debug_draw.hpp"
#include "gl/gl_shader.hpp"
#include "gl/gl_vertex_buffer.hpp"

#include <memory>

constexpr auto line_width = 3.0f;

DebugDraw::DebugDraw()
{
  lines_vertex_buffer_  = std::make_unique<GlVertexBuffer>();
  colors_vertex_buffer_ = std::make_unique<GlVertexBuffer>();

  vertex_buffers_.push_back(lines_vertex_buffer_.get());
  vertex_buffers_.push_back(colors_vertex_buffer_.get());

  gl_shader_ = std::make_unique<GlShader>();
  gl_shader_->init("shaders/line.vert", "shaders/line.frag");
}

void DebugDraw::draw_line(const glm::vec3 &from,
                          const glm::vec3 &to,
                          const glm::vec3 &color)
{
  lines_.push_back(from);
  lines_.push_back(to);
  colors_.push_back(color);
  colors_.push_back(color);
}

void DebugDraw::draw_line(const std::vector<glm::vec3> &line,
                          const glm::vec3              &color)
{
  for (int i = 1; i < line.size(); ++i)
  {
    lines_.push_back(line[i - 1]);
    colors_.push_back(color);

    lines_.push_back(line[i]);
    colors_.push_back(color);
  }
}

void DebugDraw::submit(const glm::mat4 &view_matrix,
                       const glm::mat4 &projection_matrix)
{
  GlVertexBufferLayout layout_vec3;
  layout_vec3.push_float(3);

  lines_vertex_buffer_->set_data(lines_, layout_vec3, GL_DYNAMIC_DRAW);
  colors_vertex_buffer_->set_data(colors_, layout_vec3, GL_DYNAMIC_DRAW);

  gl_shader_->bind();
  gl_shader_->set_uniform("view_matrix", view_matrix);
  gl_shader_->set_uniform("projection_matrix", projection_matrix);
  gl_shader_->draw(vertex_buffers_, GL_LINES);
  gl_shader_->unbind();

  lines_.clear();
  colors_.clear();
}
