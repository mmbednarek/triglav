#include "Swapchain.hpp"

#include "QueueManager.hpp"
#include "Synchronization.hpp"
#include "vulkan/Util.hpp"

namespace triglav::graphics_api {

Swapchain::Swapchain(QueueManager& queueManager, const Resolution& resolution, std::vector<Texture> textures,
                     vulkan::SwapchainKHR swapchain, const ColorFormat& colorFormat) :
    m_queueManager(queueManager),
    m_resolution(resolution),
    m_textures(std::move(textures)),
    m_swapchain(std::move(swapchain)),
    m_colorFormat(colorFormat)
{
}

VkSwapchainKHR Swapchain::vulkan_swapchain() const
{
   return *m_swapchain;
}

Result<std::tuple<u32, bool>> Swapchain::get_available_framebuffer(const Semaphore& semaphore) const
{
   u32 imageIndex;
   if (const auto res =
          vkAcquireNextImageKHR(m_swapchain.parent(), *m_swapchain, UINT64_MAX, semaphore.vulkan_semaphore(), VK_NULL_HANDLE, &imageIndex);
       res != VK_SUCCESS) {
      if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
         return std::make_tuple(imageIndex, true);
      }
      return std::unexpected{Status::UnsupportedDevice};
   }
   return std::make_tuple(imageIndex, false);
}

Resolution Swapchain::resolution() const
{
   return m_resolution;
}

Status Swapchain::present(const SemaphoreArrayView& waitSemaphores, const uint32_t framebufferIndex)
{
   const std::array swapchains{*m_swapchain};
   const std::array imageIndices{framebufferIndex};

   VkPresentInfoKHR presentInfo{};
   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   presentInfo.waitSemaphoreCount = static_cast<u32>(waitSemaphores.semaphore_count());
   presentInfo.pWaitSemaphores = waitSemaphores.vulkan_semaphores();
   presentInfo.swapchainCount = static_cast<u32>(swapchains.size());
   presentInfo.pSwapchains = swapchains.data();
   presentInfo.pImageIndices = imageIndices.data();
   presentInfo.pResults = nullptr;

   auto& queue = m_queueManager.get().next_queue(WorkType::Presentation);

   auto queueAccessor = queue.access();
   if (auto status = vkQueuePresentKHR(*queueAccessor, &presentInfo); status != VK_SUCCESS) {
      if (status == VK_ERROR_OUT_OF_DATE_KHR || status == VK_SUBOPTIMAL_KHR) {
         return Status::OutOfDateSwapchain;
      }
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

VkImageView Swapchain::vulkan_image_view(const u32 frameIndex) const
{
   return m_textures[frameIndex].vulkan_image_view();
}

u32 Swapchain::frame_count() const
{
   return static_cast<u32>(m_textures.size());
}

ColorFormat Swapchain::color_format() const
{
   return m_colorFormat;
}

const std::vector<Texture>& Swapchain::textures() const
{
   return m_textures;
}

}// namespace triglav::graphics_api