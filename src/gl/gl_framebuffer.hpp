#pragma once

#include "gl.hpp"
#include "gl_renderbuffer.hpp"
#include "gl_texture.hpp"

#include <memory>
#include <optional>
#include <variant>
#include <vector>

enum class AttachmentType
{
  Texture,
  Renderbuffer,
};

struct FramebufferAttachment
{
  AttachmentType type_;
  GLenum         format_;
  GLint          internal_format_;
  GLsizei        width_;
  GLsizei        height_;
};

struct FramebufferConfig
{
  std::vector<FramebufferAttachment>   color_attachments_;
  std::optional<FramebufferAttachment> depth_attachment_;
  std::optional<FramebufferAttachment> stencil_attachment_;
};

class GlFramebuffer
{
public:
  GlFramebuffer();
  ~GlFramebuffer();

  void bind();
  void unbind();

  void attach(const FramebufferConfig &config);

private:
  GLuint id_{};

  using TextureAttachment      = std::unique_ptr<GlTexture>;
  using RenderbufferAttachment = std::unique_ptr<GlRenderbuffer>;
  using Attachment = std::variant<TextureAttachment, RenderbufferAttachment>;

  std::vector<Attachment> color_attachments_;
  Attachment              depth_attachment_;
  Attachment              stencil_attachment_;

  GlFramebuffer(const GlFramebuffer &other) = delete;
  void operator=(const GlFramebuffer &other) = delete;
  GlFramebuffer(GlFramebuffer &&other)       = delete;
  void operator=(GlFramebuffer &&other) = delete;
};
