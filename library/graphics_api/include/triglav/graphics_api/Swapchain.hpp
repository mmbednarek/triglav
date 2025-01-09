#pragma once

#include "Texture.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <optional>

namespace triglav::graphics_api {

class RenderTarget;
class Semaphore;
class QueueManager;

DECLARE_VLK_WRAPPED_CHILD_OBJECT(SwapchainKHR, Device)

class Swapchain
{
 public:
   Swapchain(QueueManager& queueManager, const Resolution& resolution, std::vector<TextureView> imageViews,
             std::vector<SwapchainTexture> textures, vulkan::SwapchainKHR swapchain, const ColorFormat& colorFormat);

   [[nodiscard]] Resolution resolution() const;

   [[nodiscard]] VkSwapchainKHR vulkan_swapchain() const;
   [[nodiscard]] Result<std::tuple<u32, bool>> get_available_framebuffer(const Semaphore& semaphore) const;
   [[nodiscard]] Status present(const Semaphore& semaphore, uint32_t framebufferIndex);
   [[nodiscard]] VkImageView vulkan_image_view(u32 frameIndex) const;
   [[nodiscard]] u32 frame_count() const;
   [[nodiscard]] ColorFormat color_format() const;
   [[nodiscard]] const std::vector<TextureView>& texture_views() const;
   [[nodiscard]] const std::vector<SwapchainTexture>& textures() const;

 private:
   std::reference_wrapper<QueueManager> m_queueManager;
   Resolution m_resolution;
   std::vector<TextureView> m_textureViews;
   std::vector<SwapchainTexture> m_textures;
   vulkan::SwapchainKHR m_swapchain;
   ColorFormat m_colorFormat;
};

}// namespace triglav::graphics_api