#pragma once

#include <glad/glad.h>

#include <vector>

struct GlVertexBufferLayoutElement
{
  GLsizei stride;
};

class GlVertexBufferLayout
{
public:
  void push_float(unsigned count);
  void push_int(unsigned count);

  std::vector<GlVertexBufferLayoutElement> elements() const;

private:
  std::vector<GlVertexBufferLayoutElement> elements_;
};

class GlVertexBuffer
{
public:
  GlVertexBuffer();
  ~GlVertexBuffer();

  GLuint id() const;

  template <typename T>
  void set_data(const std::vector<T>      &data,
                const GlVertexBufferLayout layout,
                GLenum                     usage = GL_STATIC_DRAW)
  {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id_);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    vertex_count_ = data.size();
    layout_       = layout;
  }

  GlVertexBufferLayout layout() const;

  GLsizei count() const;

private:
  GLuint vertex_buffer_id_{};

  GlVertexBufferLayout layout_;

  GLsizei vertex_count_{};

  GlVertexBuffer(const GlVertexBuffer &) = delete;
  void operator=(const GlVertexBuffer &) = delete;
  GlVertexBuffer(GlVertexBuffer &&)      = delete;
  void operator=(GlVertexBuffer &&) = delete;
};
