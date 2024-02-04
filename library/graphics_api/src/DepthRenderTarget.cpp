#include "DepthRenderTarget.h"

#include "RenderPass.h"
#include "vulkan/Util.h"

namespace graphics_api {

DepthRenderTarget::DepthRenderTarget(const VkDevice device,
                                     const ColorFormat &depthFormat) :
    m_device(device),
    m_depthFormat(depthFormat)
{
}

Subpass DepthRenderTarget::vulkan_subpass()
{
   Subpass subpass;
   subpass.references.resize(1);

   auto &depthAttachment      = subpass.references[0];
   depthAttachment.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
   depthAttachment.attachment = 0;

   subpass.description.pDepthStencilAttachment = &depthAttachment;

   return subpass;
}

std::vector<VkAttachmentDescription> DepthRenderTarget::vulkan_attachments()
{
   std::vector<VkAttachmentDescription> result{};
   result.resize(1);

   auto &depthAttachment          = result[0];
   depthAttachment.format         = GAPI_CHECK(vulkan::to_vulkan_color_format(m_depthFormat));
   depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
   depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
   depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
   depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
   depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

   return result;
}

std::vector<VkSubpassDependency> DepthRenderTarget::vulkan_subpass_dependencies()
{
   std::vector<VkSubpassDependency> dependencies;
   dependencies.resize(2);

   dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
   dependencies[0].dstSubpass      = 0;
   dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
   dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
   dependencies[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
   dependencies[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
   dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

   dependencies[1].srcSubpass      = 0;
   dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
   dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
   dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
   dependencies[1].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
   dependencies[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
   dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

   return dependencies;
}

SampleCount DepthRenderTarget::sample_count() const
{
   return SampleCount::Single;
}

int DepthRenderTarget::color_attachment_count() const
{
   return 0;
}

Result<Framebuffer> DepthRenderTarget::create_framebuffer(const RenderPass &renderPass,
                                                          const Texture &texture) const
{
   assert(texture.type() == TextureType::SampledDepthBuffer);

   const std::array attachmentImageViews{texture.vulkan_image_view()};

   VkFramebufferCreateInfo framebufferInfo{};
   framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebufferInfo.renderPass      = renderPass.vulkan_render_pass();
   framebufferInfo.attachmentCount = attachmentImageViews.size();
   framebufferInfo.pAttachments    = attachmentImageViews.data();
   framebufferInfo.width           = texture.width();
   framebufferInfo.height          = texture.height();
   framebufferInfo.layers          = 1;

   vulkan::Framebuffer framebuffer(m_device);
   if (framebuffer.construct(&framebufferInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Framebuffer(texture.resolution(), renderPass.vulkan_render_pass(), std::move(framebuffer));
}

}// namespace graphics_api