#pragma once

#include "Framebuffer.h"
#include "IRenderTarget.hpp"
#include "Texture.h"

namespace graphics_api {

class RenderPass;

class DepthRenderTarget final : public IRenderTarget
{
 public:
   DepthRenderTarget(VkDevice device, const Resolution &resolution, const ColorFormat &depthFormat);

   [[nodiscard]] Subpass vulkan_subpass() override;
   [[nodiscard]] std::vector<VkAttachmentDescription> vulkan_attachments() override;
   [[nodiscard]] std::vector<VkSubpassDependency> vulkan_subpass_dependencies() override;
   [[nodiscard]] Resolution resolution() const override;
   [[nodiscard]] SampleCount sample_count() const override;
   [[nodiscard]] Result<Framebuffer> create_framebuffer(const RenderPass &renderPass,
                                                        const Texture & texture) const;

 private:
   VkDevice m_device;
   Resolution m_resolution;
   ColorFormat m_depthFormat{};
};

}// namespace graphics_api