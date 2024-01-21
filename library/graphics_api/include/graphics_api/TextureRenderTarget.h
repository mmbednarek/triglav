#pragma once

#include <span>

#include "Framebuffer.h"
#include "IRenderTarget.hpp"
#include "Texture.h"

namespace graphics_api {

class RenderPass;

class TextureRenderTarget final : public IRenderTarget
{
 public:
   TextureRenderTarget(VkDevice device, const Resolution &resolution);

   [[nodiscard]] Subpass vulkan_subpass() override;
   [[nodiscard]] std::vector<VkAttachmentDescription> vulkan_attachments() override;
   [[nodiscard]] std::vector<VkSubpassDependency> vulkan_subpass_dependencies() override;
   [[nodiscard]] Resolution resolution() const override;
   [[nodiscard]] SampleCount sample_count() const override;
   [[nodiscard]] int color_attachment_count() const override;
   [[nodiscard]] Result<Framebuffer> create_framebuffer_raw(const RenderPass &renderPass,
                                                            const std::span<const Texture *> &textures) const;

   template<typename... TTextures>
   [[nodiscard]] Result<Framebuffer> create_framebuffer(const RenderPass &renderPass,
                                                        TTextures &...textures) const
   {
      std::array<const Texture *, sizeof...(textures)> attachments{(&textures)...};
      return this->create_framebuffer_raw(renderPass, attachments);
   }

   void add_attachment(AttachmentType type, AttachmentLifetime lifetime, const ColorFormat &format,
                       SampleCount sampleCount);

 private:
   struct Attachment
   {
      AttachmentType type;
      AttachmentLifetime lifetime;
      ColorFormat format;
      SampleCount sampleCount;
   };

   VkDevice m_device;
   Resolution m_resolution;
   std::vector<Attachment> m_attachments;
};

}// namespace graphics_api