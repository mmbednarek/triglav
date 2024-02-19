#include "TextureRenderTarget.h"

#include <optional>

#include "RenderPass.h"
#include "vulkan/Util.h"

namespace triglav::graphics_api {

TextureRenderTarget::TextureRenderTarget(const VkDevice device) :
    m_device(device)
{
}

Subpass TextureRenderTarget::vulkan_subpass()
{
   Subpass result{};
   result.description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

   result.references.resize(m_attachments.size());
   uint32_t referenceIndex = 0;
   std::optional<uint32_t> depthAttachmentIndex;
   for (uint32_t i = 0; i < m_attachments.size(); ++i) {
      const auto type = m_attachments[i].type;
      if (type == AttachmentType::Color || type == AttachmentType::Presentable) {
         result.references[referenceIndex] =
                 VkAttachmentReference{i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
         ++referenceIndex;
      } else if (type == AttachmentType::Depth) {
         depthAttachmentIndex.emplace(i);
      }
   }

   result.description.colorAttachmentCount = referenceIndex;
   result.description.pColorAttachments    = &result.references[0];

   if (depthAttachmentIndex.has_value()) {
      result.references[referenceIndex] =
              VkAttachmentReference{*depthAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
      result.description.pDepthStencilAttachment = &result.references[referenceIndex];
      ++referenceIndex;
   }

   return result;
}

std::vector<VkAttachmentDescription> TextureRenderTarget::vulkan_attachments()
{
   std::vector<VkAttachmentDescription> result{};
   result.resize(m_attachments.size());

   for (size_t i = 0; i < m_attachments.size(); ++i) {
      const auto [attachmentType, attachmentLifetime, attachmentFormat, sampleCount] = m_attachments[i];
      const auto [vulkanLoadOp, vulkanStoreOp] = vulkan::to_vulkan_load_store_ops(attachmentLifetime);

      auto &attachment          = result[i];
      attachment.format         = GAPI_CHECK(vulkan::to_vulkan_color_format(attachmentFormat));
      attachment.samples        = GAPI_CHECK(vulkan::to_vulkan_sample_count(sampleCount));
      attachment.loadOp         = vulkanLoadOp;
      attachment.storeOp        = vulkanStoreOp;
      attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
      attachment.finalLayout = GAPI_CHECK(vulkan::to_vulkan_image_layout(attachmentType, attachmentLifetime));
   }

   return result;
}

std::vector<VkSubpassDependency> TextureRenderTarget::vulkan_subpass_dependencies()
{
   bool hasDepthSrcExternalDependency = false;
   bool hasDepthDstExternalDependency = false;
   bool hasColorSrcExternalDependency = false;

   for (const auto &attachment : m_attachments) {
      if (attachment.type == AttachmentType::Color &&
          attachment.lifetime == AttachmentLifetime::ClearPreserve) {
         hasColorSrcExternalDependency = true;
      }
      if (attachment.type == AttachmentType::Depth) {
         if (attachment.lifetime == AttachmentLifetime::ClearPreserve) {
            hasDepthSrcExternalDependency = true;
            hasDepthDstExternalDependency = true;
         } else if (attachment.lifetime == AttachmentLifetime::ClearDiscard) {
            hasDepthSrcExternalDependency = true;
         }
      }
   }


   std::vector<VkSubpassDependency> dependencies;
   dependencies.reserve(4);

   if (hasDepthSrcExternalDependency) {
      VkSubpassDependency dependency;
      dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass      = 0;
      dependency.srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependency.dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependency.srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
      dependency.dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
      dependencies.push_back(dependency);
   }

   if (hasColorSrcExternalDependency) {
      VkSubpassDependency dependency;
      dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass      = 0;
      dependency.srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
      dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
      dependencies.push_back(dependency);
   }

   if (hasDepthDstExternalDependency) {
      VkSubpassDependency dependency;
      dependency.srcSubpass      = 0;
      dependency.dstSubpass      = VK_SUBPASS_EXTERNAL;
      dependency.srcStageMask    = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependency.dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependency.srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependency.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
      dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
      dependencies.push_back(dependency);
   }

   return dependencies;
}

SampleCount TextureRenderTarget::sample_count() const
{
   return SampleCount::Single;
}

int TextureRenderTarget::color_attachment_count() const
{
   int count = 0;
   for (const auto &attachment : m_attachments) {
      if (attachment.type == AttachmentType::Color || attachment.type == AttachmentType::Presentable) {
         ++count;
      }
   }
   return count;
}

Result<Framebuffer>
TextureRenderTarget::create_framebuffer_raw(const RenderPass &renderPass,
                                            const std::span<const Texture *> &textures) const
{
   std::vector<VkImageView> attachmentImageViews;
   attachmentImageViews.resize(textures.size());

   auto resolution = textures[0]->resolution();

   for (size_t i = 0; i < textures.size(); ++i) {
      attachmentImageViews[i] = textures[i]->vulkan_image_view();
   }

   VkFramebufferCreateInfo framebufferInfo{};
   framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebufferInfo.renderPass      = renderPass.vulkan_render_pass();
   framebufferInfo.attachmentCount = attachmentImageViews.size();
   framebufferInfo.pAttachments    = attachmentImageViews.data();
   framebufferInfo.width           = resolution.width;
   framebufferInfo.height          = resolution.height;
   framebufferInfo.layers          = 1;

   vulkan::Framebuffer framebuffer(m_device);
   if (framebuffer.construct(&framebufferInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Framebuffer(resolution, renderPass.vulkan_render_pass(), std::move(framebuffer));
}

void TextureRenderTarget::add_attachment(const AttachmentType type, const AttachmentLifetime lifetime,
                                         const ColorFormat &format, const SampleCount sampleCount)
{
   m_attachments.emplace_back(type, lifetime, format, sampleCount);
}

}// namespace graphics_api