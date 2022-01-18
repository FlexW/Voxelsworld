#include "gl_index_buffer.hpp"

GlIndexBuffer::GlIndexBuffer() { glGenBuffers(1, &index_buffer_id_); }

GlIndexBuffer::~GlIndexBuffer()
{
  if (index_buffer_id_)
  {
    glDeleteBuffers(1, &index_buffer_id_);
  }
}

GLuint GlIndexBuffer::id() const { return index_buffer_id_; }

void GlIndexBuffer::set_data(const std::vector<unsigned> &indices)
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               indices.size() * sizeof(unsigned),
               indices.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  count_ = indices.size();
}

GLsizei GlIndexBuffer::count() const { return count_; }
