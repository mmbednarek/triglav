#include "Swapchain.h"

#include "QueueManager.h"
#include "Synchronization.h"
#include "vulkan/Util.h"

namespace triglav::graphics_api {

Swapchain::Swapchain(QueueManager& queueManager, const Resolution& resolution, std::vector<vulkan::ImageView> imageViews,
                     vulkan::SwapchainKHR swapchain, const ColorFormat& colorFormat) :
    m_queueManager(queueManager),
    m_resolution(resolution),
    m_imageViews(std::move(imageViews)),
    m_swapchain(std::move(swapchain)),
    m_colorFormat(colorFormat)
{
}

VkSwapchainKHR Swapchain::vulkan_swapchain() const
{
   return *m_swapchain;
}

Result<u32> Swapchain::get_available_framebuffer(const Semaphore& semaphore) const
{
   u32 imageIndex;
   if (const auto res =
          vkAcquireNextImageKHR(m_swapchain.parent(), *m_swapchain, UINT64_MAX, semaphore.vulkan_semaphore(), VK_NULL_HANDLE, &imageIndex);
       res != VK_SUCCESS) {
      if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
         return std::unexpected{Status::OutOfDateSwapchain};
      }
      return std::unexpected{Status::UnsupportedDevice};
   }
   return imageIndex;
}

Resolution Swapchain::resolution() const
{
   return m_resolution;
}

Status Swapchain::present(const Semaphore& semaphore, const uint32_t framebufferIndex)
{
   const std::array waitSemaphores{semaphore.vulkan_semaphore()};
   const std::array swapchains{*m_swapchain};
   const std::array imageIndices{framebufferIndex};

   VkPresentInfoKHR presentInfo{};
   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   presentInfo.waitSemaphoreCount = waitSemaphores.size();
   presentInfo.pWaitSemaphores = waitSemaphores.data();
   presentInfo.swapchainCount = swapchains.size();
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
   return *m_imageViews[frameIndex];
}

u32 Swapchain::frame_count() const
{
   return m_imageViews.size();
}

ColorFormat Swapchain::color_format() const
{
   return m_colorFormat;
}

}// namespace triglav::graphics_api