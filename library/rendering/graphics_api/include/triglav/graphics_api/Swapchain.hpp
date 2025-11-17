#pragma once

#include "Texture.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <functional>
#include <optional>

namespace triglav::graphics_api {

class Semaphore;
class SemaphoreArrayView;
class QueueManager;

DECLARE_VLK_WRAPPED_CHILD_OBJECT(SwapchainKHR, Device)

class Swapchain
{
 public:
   Swapchain(QueueManager& queue_manager, const Resolution& resolution, std::vector<Texture> textures, vulkan::SwapchainKHR swapchain,
             const ColorFormat& color_format);

   [[nodiscard]] Resolution resolution() const;

   [[nodiscard]] VkSwapchainKHR vulkan_swapchain() const;
   [[nodiscard]] Result<std::tuple<u32, bool>> get_available_framebuffer(const Semaphore& semaphore) const;
   [[nodiscard]] Status present(const SemaphoreArrayView& wait_semaphores, uint32_t framebuffer_index);
   [[nodiscard]] VkImageView vulkan_image_view(u32 frame_index) const;
   [[nodiscard]] u32 frame_count() const;
   [[nodiscard]] ColorFormat color_format() const;
   [[nodiscard]] const std::vector<Texture>& textures() const;

 private:
   std::reference_wrapper<QueueManager> m_queue_manager;
   Resolution m_resolution;
   std::vector<Texture> m_textures;
   vulkan::SwapchainKHR m_swapchain;
   ColorFormat m_color_format;
};

}// namespace triglav::graphics_api