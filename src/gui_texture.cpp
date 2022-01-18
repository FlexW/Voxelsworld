#include "gui_texture.hpp"
#include "gl_index_buffer.hpp"
#include "gl_shader.hpp"
#include "gl_vertex_buffer.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_int2.hpp"

#include <memory>

void GuiTexture::set_texture(std::shared_ptr<GlTexture> value)
{
  if (!gui_texture_shader_)
  {
    gui_texture_shader_ = std::make_unique<GlShader>();
    gui_texture_shader_->init("shaders/gui_texture.vert",
                              "shaders/gui_texture.frag");
  }

  if (!quad_vertex_buffer_)
  {
    quad_vertex_buffer_ = std::make_unique<GlVertexBuffer>();

    const std::vector<glm::vec2> vertices{
        {0.0f, 1.0f},
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    GlVertexBufferLayout layout;
    layout.push_float(2);

    quad_vertex_buffer_->set_data(vertices, layout);

    quad_index_buffer_ = std::make_unique<GlIndexBuffer>();

    const std::vector<unsigned> indices{
        0,
        1,
        2,
        2,
        3,
        0,
    };

    quad_index_buffer_->set_data(indices);
  }

  texture_ = value;
}

void GuiTexture::draw(float window_width, float window_height)
{
  if (!gui_texture_shader_ || !quad_vertex_buffer_ || !quad_index_buffer_ ||
      !texture_)
  {
    return;
  }

  auto model_matrix = glm::mat4{1.0f};
  model_matrix      = glm::translate(model_matrix, glm::vec3{position_, 0.0f});
  model_matrix = glm::scale(model_matrix, glm::vec3{width_, height_, 1.0f});

  const auto projection_matrix =
      glm::ortho(0.0f, window_width, window_height, 0.0f, -1.0f, 1.0f);

  gui_texture_shader_->bind();
  glActiveTexture(GL_TEXTURE0);
  texture_->bind();
  gui_texture_shader_->set_uniform("tex", 0);
  gui_texture_shader_->set_uniform("model_matrix", model_matrix);
  gui_texture_shader_->set_uniform("projection_matrix", projection_matrix);
  gui_texture_shader_->draw({quad_vertex_buffer_.get()},
                            *quad_index_buffer_,
                            GL_TRIANGLES);
  gui_texture_shader_->unbind();
  texture_->unbind();
}

void GuiTexture::set_position(const glm::vec2 &value) { position_ = value; }

void GuiTexture::set_width(float value) { width_ = value; }

void GuiTexture::set_height(float value) { height_ = value; }
