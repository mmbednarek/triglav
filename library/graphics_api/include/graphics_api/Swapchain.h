#pragma once

#include "Framebuffer.h"
#include "IRenderTarget.hpp"
#include "PlatformSurface.h"
#include "Texture.h"
#include "vulkan/ObjectWrapper.hpp"

#include <optional>

namespace graphics_api {

class RenderPass;
class Semaphore;

DECLARE_VLK_WRAPPED_CHILD_OBJECT(SwapchainKHR, Device)

class Swapchain final : public IRenderTarget
{
 public:
   Swapchain(const ColorFormat &colorFormat, const ColorFormat &depthFormat, SampleCount sampleCount,
             const Resolution &resolution, Texture depthAttachment, std::optional<Texture> colorAttachment,
             std::vector<vulkan::ImageView> imageViews, VkQueue presentQueue,
             vulkan::SwapchainKHR swapchain);

   [[nodiscard]] Subpass vulkan_subpass() override;
   [[nodiscard]] std::vector<VkAttachmentDescription> vulkan_attachments() override;
   [[nodiscard]] VkSwapchainKHR vulkan_swapchain() const;
   [[nodiscard]] uint32_t get_available_framebuffer(const Semaphore &semaphore) const;
   [[nodiscard]] Resolution resolution() const override;
   [[nodiscard]] ColorFormat color_format() const override;
   [[nodiscard]] SampleCount sample_count() const override;

   [[nodiscard]] Result<std::vector<Framebuffer>> create_framebuffers(const RenderPass &renderPass);
   [[nodiscard]] Status present(const Semaphore &semaphore, uint32_t framebufferIndex);

 private:
   ColorFormat m_colorFormat;
   ColorFormat m_depthFormat;
   SampleCount m_sampleCount;
   Resolution m_resolution;
   Texture m_depthAttachment;
   std::optional<Texture> m_colorAttachment;// std::nullopt if multisampling is disabled
   std::vector<vulkan::ImageView> m_imageViews;
   VkQueue m_presentQueue;

   vulkan::SwapchainKHR m_swapchain;
};

}// namespace graphics_api