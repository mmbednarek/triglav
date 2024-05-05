#include "RenderTarget.h"

#include <optional>
#include <stdexcept>

#include "Device.h"
#include "vulkan/Util.h"

namespace triglav::graphics_api {

namespace {

TextureUsageFlags to_texture_usage_flags(const AttachmentAttributeFlags attributeFlags)
{
   TextureUsageFlags textureUsageFlags{};
   if (attributeFlags & AttachmentAttribute::Color || attributeFlags & AttachmentAttribute::Resolve) {
      textureUsageFlags |= TextureUsage::ColorAttachment;
   } else if (attributeFlags & AttachmentAttribute::Depth) {
      textureUsageFlags |= TextureUsage::DepthStencilAttachment;
   }

   if (attributeFlags & AttachmentAttribute::StoreImage) {
      textureUsageFlags |= TextureUsage::Sampled;
   } else if (!(attributeFlags & AttachmentAttribute::LoadImage)) {
      textureUsageFlags |= TextureUsage::Transient;
   }

   return textureUsageFlags;
}

}// namespace

RenderTarget::RenderTarget(Device &device, vulkan::RenderPass renderPass, SampleCount sampleCount,
                           std::vector<AttachmentInfo> attachments) :
    m_device(device),
    m_renderPass(std::move(renderPass)),
    m_sampleCount(sampleCount),
    m_attachments(std::move(attachments))
{
}

SampleCount RenderTarget::sample_count() const
{
   return m_sampleCount;
}

int RenderTarget::color_attachment_count() const
{
   int count = 0;
   for (const auto &attachment : m_attachments) {
      if (attachment.flags & AttachmentAttribute::Color) {
         ++count;
      }
   }
   return count;
}

VkRenderPass RenderTarget::vulkan_render_pass() const
{
   return *m_renderPass;
}

const std::vector<AttachmentInfo> &RenderTarget::attachments() const
{
   return m_attachments;
}

Result<Framebuffer> RenderTarget::create_framebuffer(const Resolution &resolution) const
{
   Heap<Name, Texture> textures;
   for (const auto &attachment : m_attachments) {
      auto texture =
              m_device.create_texture(attachment.format, resolution, to_texture_usage_flags(attachment.flags),
                                      attachment.sampleCount);
      if (not texture.has_value()) {
         return std::unexpected{texture.error()};
      }
      textures.emplace(attachment.identifier, std::move(*texture));
   }

   textures.make_heap();

   std::vector<VkImageView> attachmentImageViews;
   attachmentImageViews.resize(textures.size());

   for (size_t i = 0; i < textures.size(); ++i) {
      attachmentImageViews[i] = textures[m_attachments[i].identifier].vulkan_image_view();
   }

   VkFramebufferCreateInfo framebufferInfo{};
   framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebufferInfo.renderPass      = *m_renderPass;
   framebufferInfo.attachmentCount = attachmentImageViews.size();
   framebufferInfo.pAttachments    = attachmentImageViews.data();
   framebufferInfo.width           = resolution.width;
   framebufferInfo.height          = resolution.height;
   framebufferInfo.layers          = 1;

   vulkan::Framebuffer framebuffer(m_device.vulkan_device());
   if (framebuffer.construct(&framebufferInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Framebuffer(resolution, *m_renderPass, std::move(framebuffer), std::move(textures));
}

Result<Framebuffer> RenderTarget::create_swapchain_framebuffer(const Swapchain &swapchain,
                                                               const u32 frameIndex) const
{
   u32 swapchainAttachment{};
   u32 index{};
   for (const auto &attachment : m_attachments) {
      if (attachment.flags & AttachmentAttribute::Presentable) {
         swapchainAttachment = index;
         break;
      }

      ++index;
   }

   index = 0;
   Heap<Name, Texture> textures;
   for (const auto &attachment : m_attachments) {
      if (index == swapchainAttachment) {
         ++index;
         continue;
      }

      auto texture =
              m_device.create_texture(attachment.format, swapchain.resolution(),
                                      to_texture_usage_flags(attachment.flags), attachment.sampleCount);
      if (not texture.has_value()) {
         return std::unexpected{texture.error()};
      }
      textures.emplace(attachment.identifier, std::move(*texture));
   }

   textures.make_heap();

   std::vector<VkImageView> attachmentImageViews;
   attachmentImageViews.resize(textures.size() + 1);

   index = 0;
   for (size_t i = 0; i < textures.size(); ++i) {
      if (index == swapchainAttachment) {
         attachmentImageViews[index] = swapchain.vulkan_image_view(frameIndex);
         ++index;
      }
      attachmentImageViews[index] = textures[m_attachments[index].identifier].vulkan_image_view();
      ++index;
   }

   VkFramebufferCreateInfo framebufferInfo{};
   framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebufferInfo.renderPass      = *m_renderPass;
   framebufferInfo.attachmentCount = attachmentImageViews.size();
   framebufferInfo.pAttachments    = attachmentImageViews.data();
   framebufferInfo.width           = swapchain.resolution().width;
   framebufferInfo.height          = swapchain.resolution().height;
   framebufferInfo.layers          = 1;

   vulkan::Framebuffer framebuffer(m_device.vulkan_device());
   if (framebuffer.construct(&framebufferInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Framebuffer(swapchain.resolution(), *m_renderPass, std::move(framebuffer), std::move(textures));
}

RenderTargetBuilder::RenderTargetBuilder(Device &device) :
    m_device(device)
{
}

RenderTargetBuilder &RenderTargetBuilder::attachment(const Name identifier,
                                                     const AttachmentAttributeFlags flags,
                                                     const ColorFormat &format, const SampleCount sampleCount)
{
   assert(!(flags & AttachmentAttribute::Depth) || !(flags & AttachmentAttribute::Color));
   assert(!(flags & AttachmentAttribute::Color) || !(flags & AttachmentAttribute::Resolve));
   assert(!(flags & AttachmentAttribute::LoadImage) || !(flags & AttachmentAttribute::ClearImage));

   if (sampleCount != SampleCount::Single) {
      if (m_sampleCount != SampleCount::Single) {
         assert(sampleCount == m_sampleCount);
      } else {
         m_sampleCount = sampleCount;
      }
   }
   m_attachments.emplace_back(identifier, flags, format, sampleCount);
   return *this;
}

Subpass RenderTargetBuilder::vulkan_subpass() const
{
   Subpass result{};
   result.description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

   result.references.resize(m_attachments.size());
   uint32_t referenceIndex = 0;
   std::optional<uint32_t> depthAttachmentIndex;
   for (uint32_t i = 0; i < m_attachments.size(); ++i) {
      const auto type = m_attachments[i].flags;
      if (type & AttachmentAttribute::Color) {
         result.references[referenceIndex] =
                 VkAttachmentReference{i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
         ++referenceIndex;
      } else if (type & AttachmentAttribute::Depth) {
         depthAttachmentIndex.emplace(i);
      }
   }

   result.description.colorAttachmentCount = referenceIndex;
   if (referenceIndex != 0) {
      result.description.pColorAttachments = &result.references[0];
   }

   if (depthAttachmentIndex.has_value()) {
      result.references[referenceIndex] =
              VkAttachmentReference{*depthAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
      result.description.pDepthStencilAttachment = &result.references[referenceIndex];
      ++referenceIndex;
   }

   return result;
}

std::vector<VkAttachmentDescription> RenderTargetBuilder::vulkan_attachments()
{
   std::vector<VkAttachmentDescription> result{};
   result.resize(m_attachments.size());

   for (size_t i = 0; i < m_attachments.size(); ++i) {
      const auto [name, attachmentFlags, attachmentFormat, sampleCount] = m_attachments[i];
      const auto [vulkanLoadOp, vulkanStoreOp] = vulkan::to_vulkan_load_store_ops(attachmentFlags);

      auto &attachment          = result[i];
      attachment.format         = GAPI_CHECK(vulkan::to_vulkan_color_format(attachmentFormat));
      attachment.samples        = GAPI_CHECK(vulkan::to_vulkan_sample_count(sampleCount));
      attachment.loadOp         = vulkanLoadOp;
      attachment.storeOp        = vulkanStoreOp;
      attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
      attachment.finalLayout    = GAPI_CHECK(vulkan::to_vulkan_image_layout(attachmentFlags));
   }

   return result;
}

std::vector<VkSubpassDependency> RenderTargetBuilder::vulkan_subpass_dependencies() const
{
   bool hasDepthSrcExternalDependency = false;
   bool hasDepthDstExternalDependency = false;
   bool hasColorSrcExternalDependency = false;

   for (const auto &attachment : m_attachments) {
      if (attachment.flags & AttachmentAttribute::Color &&
          attachment.flags & AttachmentAttribute::ClearImage) {
         hasColorSrcExternalDependency = true;
      }
      if (attachment.flags & AttachmentAttribute::Depth) {
         if (attachment.flags & AttachmentAttribute::ClearImage) {
            hasDepthSrcExternalDependency = true;
         }
         if (attachment.flags & AttachmentAttribute::StoreImage) {
            hasDepthDstExternalDependency = true;
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

Result<RenderTarget> RenderTargetBuilder::build()
{
   const auto attachments  = this->vulkan_attachments();
   const auto subpass      = this->vulkan_subpass();
   const auto dependencies = this->vulkan_subpass_dependencies();

   VkRenderPassCreateInfo renderPassInfo{};
   renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = attachments.size();
   renderPassInfo.pAttachments    = attachments.data();
   renderPassInfo.subpassCount    = 1;
   renderPassInfo.pSubpasses      = &subpass.description;
   renderPassInfo.dependencyCount = dependencies.size();
   renderPassInfo.pDependencies   = dependencies.data();

   vulkan::RenderPass renderPass(m_device.vulkan_device());
   if (const auto res = renderPass.construct(&renderPassInfo); res != VK_SUCCESS) {
      return std::unexpected{Status::UnsupportedDevice};
   }

   return RenderTarget(m_device, std::move(renderPass), m_sampleCount, std::move(m_attachments));
}

}// namespace triglav::graphics_api
