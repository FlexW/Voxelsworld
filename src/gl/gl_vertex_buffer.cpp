#include "gl_vertex_buffer.hpp"

void GlVertexBufferLayout::push_float(unsigned count)
{
  GlVertexBufferLayoutElement element{};
  element.stride = count * sizeof(float);
  elements_.push_back(element);
}

void GlVertexBufferLayout::push_int(unsigned count)
{
  GlVertexBufferLayoutElement element{};
  element.stride = count * sizeof(int);
  elements_.push_back(element);
}

std::vector<GlVertexBufferLayoutElement> GlVertexBufferLayout::elements() const
{
  return elements_;
}

GlVertexBuffer::GlVertexBuffer() { glGenBuffers(1, &vertex_buffer_id_); }

GlVertexBuffer::~GlVertexBuffer()
{
  if (vertex_buffer_id_)
  {
    glDeleteBuffers(1, &vertex_buffer_id_);
  }
}

GLuint GlVertexBuffer::id() const { return vertex_buffer_id_; }

GlVertexBufferLayout GlVertexBuffer::layout() const { return layout_; }

GLsizei GlVertexBuffer::count() const { return vertex_count_; }
