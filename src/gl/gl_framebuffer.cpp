#include "gl_framebuffer.hpp"
#include "gl_renderbuffer.hpp"
#include "gl_texture.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

GlFramebuffer::GlFramebuffer() { glGenFramebuffers(1, &id_); }

GlFramebuffer::~GlFramebuffer() { glDeleteFramebuffers(1, &id_); }

void GlFramebuffer::bind() { glBindFramebuffer(GL_FRAMEBUFFER, id_); }

void GlFramebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void GlFramebuffer::attach(const FramebufferConfig &config)
{
  bind();

  // Attach the color attachments
  assert(config.color_attachments_.size() <= GL_MAX_COLOR_ATTACHMENTS);
  for (std::size_t i = 0;
       i < config.color_attachments_.size() && i < GL_MAX_COLOR_ATTACHMENTS;
       ++i)
  {
    const auto &color_attachment = config.color_attachments_[i];

    switch (color_attachment.type_)
    {
    case AttachmentType::Texture:
    {
      auto texture = std::make_shared<GlTexture>();
      texture->set_storage(color_attachment.width_,
                           color_attachment.height_,
                           color_attachment.internal_format_,
                           color_attachment.format_);
      glFramebufferTexture2D(GL_FRAMEBUFFER,
                             GL_COLOR_ATTACHMENT0 + i,
                             GL_TEXTURE_2D,
                             texture->id(),
                             0);
      color_attachments_.push_back(std::move(texture));
      break;
    }
    case AttachmentType::Renderbuffer:
    {
      auto renderbuffer = std::make_shared<GlRenderbuffer>();

      renderbuffer->set_storage(color_attachment.internal_format_,
                                color_attachment.width_,
                                color_attachment.height_);

      glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                GL_COLOR_ATTACHMENT0 + i,
                                GL_RENDERBUFFER,
                                renderbuffer->id());

      color_attachments_.push_back(std::move(renderbuffer));
      break;
    }
    }
  }

  // Attach the depth attachment
  if (config.depth_attachment_.has_value())
  {
    const auto &depth_attachment = config.depth_attachment_.value();
    switch (depth_attachment.type_)
    {
    case AttachmentType::Texture:
    {
      break;
    }
    case AttachmentType::Renderbuffer:
    {
      auto renderbuffer = std::make_shared<GlRenderbuffer>();

      renderbuffer->set_storage(depth_attachment.internal_format_,
                                depth_attachment.width_,
                                depth_attachment.height_);

      glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                GL_DEPTH_ATTACHMENT,
                                GL_RENDERBUFFER,
                                renderbuffer->id());
      depth_attachment_ = std::move(renderbuffer);
      break;
    }
    }
  }

  // Attach the stencil attachment
  if (config.stencil_attachment_.has_value())
  {
    const auto &stencil_attachment = config.stencil_attachment_.value();
    switch (stencil_attachment.type_)
    {
    case AttachmentType::Texture:
    {
      break;
    }
    case AttachmentType::Renderbuffer:
    {
      auto renderbuffer = std::make_shared<GlRenderbuffer>();

      renderbuffer->set_storage(stencil_attachment.internal_format_,
                                stencil_attachment.width_,
                                stencil_attachment.height_);

      glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                GL_DEPTH_ATTACHMENT,
                                GL_RENDERBUFFER,
                                renderbuffer->id());

      stencil_attachment_ = std::move(renderbuffer);
      break;
    }
    }
  }

  // Check if the framebuffer is complete
  const auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  switch (result)
  {
  case GL_FRAMEBUFFER_COMPLETE:
  {
    // Nothing to do, all fine.
    break;
  }
  case GL_FRAMEBUFFER_UNDEFINED:
  {
    throw std::runtime_error("Bound framebuffer is undefined");
  }
  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
  {
    throw std::runtime_error(
        "A necessary attachment to the framebuffer is uninitialized");
  }
  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
  {
    throw std::runtime_error(
        "A necessary attachment to the framebuffer is missing");
  }
  case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
  {
    throw std::runtime_error("Incomplete draw buffer");
  }
  case GL_FRAMEBUFFER_UNSUPPORTED:
  {
    throw std::runtime_error("Combination of attachments is unsupported");
  }
  case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
  {
    throw std::runtime_error(
        "Number of samples for all images across framebuffer do not match");
  }
  default:
  {
    throw std::runtime_error("Unknown framebuffer error");
  }
  };

  unbind();
}

Attachment GlFramebuffer::color_attachment(std::size_t index) const
{
  if (index >= color_attachments_.size())
  {
    return {};
  }

  return color_attachments_[index];
}

Attachment GlFramebuffer::depth_attachment() const { return depth_attachment_; }

Attachment GlFramebuffer::stencil_attachment() const
{
  return stencil_attachment_;
}
