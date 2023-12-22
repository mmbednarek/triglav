#include "Swapchain.h"

#include "RenderPass.h"
#include "Synchronization.h"
#include "vulkan/Util.h"

namespace graphics_api {

constexpr size_t g_colorAttachmentIndex   = 0;
constexpr size_t g_depthAttachmentIndex   = 1;
constexpr size_t g_resolveAttachmentIndex = 2;

Swapchain::Swapchain(const ColorFormat &colorFormat, const ColorFormat &depthFormat,
                     const SampleCount sampleCount, const Resolution &resolution, Texture depthAttachment,
                     std::optional<Texture> colorAttachment, std::vector<vulkan::ImageView> imageViews,
                     VkQueue presentQueue, vulkan::SwapchainKHR swapchain) :
    m_colorFormat(colorFormat),
    m_depthFormat(depthFormat),
    m_sampleCount(sampleCount),
    m_resolution(resolution),
    m_depthAttachment(std::move(depthAttachment)),
    m_colorAttachment(std::move(colorAttachment)),
    m_imageViews(std::move(imageViews)),
    m_presentQueue(presentQueue),
    m_swapchain(std::move(swapchain))
{
}

Subpass Swapchain::vulkan_subpass()
{
   Subpass result{};
   result.references.resize(3);

   result.description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

   result.references[g_colorAttachmentIndex] =
           VkAttachmentReference{g_colorAttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   result.description.colorAttachmentCount = 1;
   result.description.pColorAttachments    = &result.references[g_colorAttachmentIndex];

   result.references[g_depthAttachmentIndex] =
           VkAttachmentReference{g_depthAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
   result.description.pDepthStencilAttachment = &result.references[g_depthAttachmentIndex];

   if (m_colorAttachment.has_value()) {
      result.references[g_resolveAttachmentIndex] =
              VkAttachmentReference{g_resolveAttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
      result.description.pResolveAttachments = &result.references[g_resolveAttachmentIndex];
   }

   return result;
}

std::vector<VkAttachmentDescription> Swapchain::vulkan_attachments()
{
   std::vector<VkAttachmentDescription> result{};

   if (m_colorAttachment.has_value()) {
      result.resize(3);

      auto &colorAttachment          = result[g_colorAttachmentIndex];
      colorAttachment.format         = GAPI_CHECK(vulkan::to_vulkan_color_format(m_colorFormat));
      colorAttachment.samples        = static_cast<VkSampleCountFlagBits>(m_sampleCount);
      colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      auto &resolveAttachment          = result[g_resolveAttachmentIndex];
      resolveAttachment.format         = GAPI_CHECK(vulkan::to_vulkan_color_format(m_colorFormat));
      resolveAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
      resolveAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
      resolveAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
      resolveAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      resolveAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
      resolveAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   } else {
      result.resize(2);
      auto &colorAttachment          = result[g_colorAttachmentIndex];
      colorAttachment.format         = GAPI_CHECK(vulkan::to_vulkan_color_format(m_colorFormat));
      colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   }

   auto &depthAttachment          = result[g_depthAttachmentIndex];
   depthAttachment.format         = GAPI_CHECK(vulkan::to_vulkan_color_format(m_depthFormat));
   depthAttachment.samples        = static_cast<VkSampleCountFlagBits>(m_sampleCount);
   depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
   depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
   depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   return result;
}

VkSwapchainKHR Swapchain::vulkan_swapchain() const
{
   return *m_swapchain;
}

uint32_t Swapchain::get_available_framebuffer(const Semaphore &semaphore) const
{
   uint32_t imageIndex;
   vkAcquireNextImageKHR(m_swapchain.parent(), *m_swapchain, UINT64_MAX, semaphore.vulkan_semaphore(),
                         VK_NULL_HANDLE, &imageIndex);
   return imageIndex;
}

Resolution Swapchain::resolution() const
{
   return m_resolution;
}

ColorFormat Swapchain::color_format() const
{
   return m_colorFormat;
}

SampleCount Swapchain::sample_count() const
{
   return m_sampleCount;
}

Result<std::vector<Framebuffer>> Swapchain::create_framebuffers(const RenderPass &renderPass)
{
   size_t attachmentCount{2};
   size_t swapchainImageIndex{0};

   std::array<VkImageView, 3> attachmentImageViews{nullptr, m_depthAttachment.vulkan_image_view()};
   if (m_colorAttachment.has_value()) {
      attachmentImageViews[0] = m_colorAttachment->vulkan_image_view();
      attachmentCount         = 3;
      swapchainImageIndex     = 2;
   }

   std::vector<Framebuffer> result{};
   for (const auto &imageView : m_imageViews) {
      attachmentImageViews[swapchainImageIndex] = *imageView;

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass      = renderPass.vulkan_render_pass();
      framebufferInfo.attachmentCount = attachmentCount;
      framebufferInfo.pAttachments    = attachmentImageViews.data();
      framebufferInfo.width           = m_resolution.width;
      framebufferInfo.height          = m_resolution.height;
      framebufferInfo.layers          = 1;

      vulkan::Framebuffer framebuffer(m_swapchain.parent());
      if (framebuffer.construct(&framebufferInfo) != VK_SUCCESS) {
         return std::unexpected(Status::UnsupportedDevice);
      }

      result.emplace_back(renderPass.resolution(), renderPass.vulkan_render_pass(), std::move(framebuffer));
   }

   return result;
}

Status Swapchain::present(const Semaphore &semaphore, const uint32_t framebufferIndex)
{
   std::array waitSemaphores{semaphore.vulkan_semaphore()};
   std::array swapchains{*m_swapchain};
   std::array imageIndices{framebufferIndex};

   VkPresentInfoKHR presentInfo{};
   presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   presentInfo.waitSemaphoreCount = waitSemaphores.size();
   presentInfo.pWaitSemaphores    = waitSemaphores.data();
   presentInfo.swapchainCount     = swapchains.size();
   presentInfo.pSwapchains        = swapchains.data();
   presentInfo.pImageIndices      = imageIndices.data();
   presentInfo.pResults           = nullptr;

   if (vkQueuePresentKHR(m_presentQueue, &presentInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

}// namespace graphics_api