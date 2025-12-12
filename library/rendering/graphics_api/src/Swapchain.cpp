#include "Swapchain.hpp"

#include "QueueManager.hpp"
#include "Synchronization.hpp"
#include "vulkan/Util.hpp"

namespace triglav::graphics_api {

Swapchain::Swapchain(QueueManager& queue_manager, const Resolution& resolution, std::vector<Texture> textures,
                     vulkan::SwapchainKHR swapchain, const ColorFormat& color_format) :
    m_queue_manager(queue_manager),
    m_resolution(resolution),
    m_textures(std::move(textures)),
    m_swapchain(std::move(swapchain)),
    m_color_format(color_format)
{
}

VkSwapchainKHR Swapchain::vulkan_swapchain() const
{
   return *m_swapchain;
}

Result<std::tuple<u32, bool>> Swapchain::get_available_framebuffer(const Semaphore& semaphore) const
{
   u32 image_index;
   if (const auto res =
          vkAcquireNextImageKHR(m_swapchain.parent(), *m_swapchain, UINT64_MAX, semaphore.vulkan_semaphore(), VK_NULL_HANDLE, &image_index);
       res != VK_SUCCESS) {
      if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
         return std::make_tuple(image_index, true);
      }
      return std::unexpected{Status::UnsupportedDevice};
   }
   return std::make_tuple(image_index, false);
}

Resolution Swapchain::resolution() const
{
   return m_resolution;
}

Status Swapchain::present(const SemaphoreArrayView& wait_semaphores, const uint32_t framebuffer_index)
{
   const std::array swapchains{*m_swapchain};
   const std::array image_indices{framebuffer_index};

   VkPresentInfoKHR present_info{};
   present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   present_info.waitSemaphoreCount = static_cast<u32>(wait_semaphores.semaphore_count());
   present_info.pWaitSemaphores = wait_semaphores.vulkan_semaphores();
   present_info.swapchainCount = static_cast<u32>(swapchains.size());
   present_info.pSwapchains = swapchains.data();
   present_info.pImageIndices = image_indices.data();
   present_info.pResults = nullptr;

   auto& queue = m_queue_manager.get().next_queue(WorkType::Presentation);

   auto queue_accessor = queue.access();
   if (auto status = vkQueuePresentKHR(*queue_accessor, &present_info); status != VK_SUCCESS) {
      if (status == VK_ERROR_OUT_OF_DATE_KHR || status == VK_SUBOPTIMAL_KHR) {
         return Status::OutOfDateSwapchain;
      }
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

VkImageView Swapchain::vulkan_image_view(const u32 frame_index) const
{
   return m_textures[frame_index].vulkan_image_view();
}

u32 Swapchain::frame_count() const
{
   return static_cast<u32>(m_textures.size());
}

ColorFormat Swapchain::color_format() const
{
   return m_color_format;
}

const std::vector<Texture>& Swapchain::textures() const
{
   return m_textures;
}

}// namespace triglav::graphics_api