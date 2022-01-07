#pragma once

#include "gl/gl_shader.hpp"
#include "gl/gl_vertex_buffer.hpp"
#include "math.hpp"

#include <memory>
#include <vector>

class DebugDraw
{
public:
  DebugDraw();

  void
  draw_line(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color);

  void submit(const glm::mat4 &view_matrix, const glm::mat4 &projection_matrix);

private:
  std::vector<glm::vec3> lines_;
  std::vector<glm::vec3> colors_;

  std::unique_ptr<GlShader> gl_shader_;

  std::unique_ptr<GlVertexBuffer>     lines_vertex_buffer_;
  std::unique_ptr<GlVertexBuffer>     colors_vertex_buffer_;
  std::vector<const GlVertexBuffer *> vertex_buffers_;
};
