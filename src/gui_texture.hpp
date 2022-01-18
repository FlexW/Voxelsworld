#pragma once

#include "gl_index_buffer.hpp"
#include "gl_shader.hpp"
#include "gl_texture.hpp"
#include "gl_vertex_buffer.hpp"
#include "gui_element.hpp"

#include <memory>

class GuiTexture : public GuiElement
{
public:
  void set_texture(std::shared_ptr<GlTexture> value);

  void set_position(const glm::vec2 &value);

  void set_width(float value);
  void set_height(float value);

  void draw(float window_width, float window_height) override;

private:
  std::unique_ptr<GlVertexBuffer> quad_vertex_buffer_{};
  std::unique_ptr<GlIndexBuffer>  quad_index_buffer_{};
  std::unique_ptr<GlShader>       gui_texture_shader_{};
  std::shared_ptr<GlTexture>      texture_{};

  glm::vec2 position_{glm::vec2{0.0f}};

  float width_{300.0f};
  float height_{300.0f};
};
