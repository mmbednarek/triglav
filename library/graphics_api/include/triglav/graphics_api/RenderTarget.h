#pragma once

#include <span>

#include "Framebuffer.h"
#include "Texture.h"
#include "Swapchain.h"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(RenderPass, Device);

class RenderTargetBuilder;
class Device;
class Swapchain;

struct AttachmentInfo
{
   AttachmentTypeFlags type;
   AttachmentLifetime lifetime;
   ColorFormat format;
   SampleCount sampleCount;
};

struct Subpass
{
   std::vector<VkAttachmentReference> references{};
   VkSubpassDescription description{};
};

class RenderTarget
{
 public:
   RenderTarget(Device &device, vulkan::RenderPass renderPass, SampleCount sampleCount,
                std::vector<AttachmentInfo> attachments);

   [[nodiscard]] SampleCount sample_count() const;
   [[nodiscard]] int color_attachment_count() const;
   [[nodiscard]] VkRenderPass vulkan_render_pass() const;

   [[nodiscard]] Result<Framebuffer> create_framebuffer(const Resolution& resolution) const;
   [[nodiscard]] Result<Framebuffer> create_swapchain_framebuffer(const Swapchain& swapchain, u32 frameIndex) const;

 private:
   Device &m_device;
   vulkan::RenderPass m_renderPass;
   SampleCount m_sampleCount{};
   std::vector<AttachmentInfo> m_attachments;
};

class RenderTargetBuilder
{
 public:
   explicit RenderTargetBuilder(Device &device);

   RenderTargetBuilder &attachment(AttachmentTypeFlags type, AttachmentLifetime lifetime, const ColorFormat &format, SampleCount sampleCount);

   Result<RenderTarget> build();

 private:
   [[nodiscard]] Subpass vulkan_subpass() const;
   [[nodiscard]] std::vector<VkAttachmentDescription> vulkan_attachments();
   [[nodiscard]] std::vector<VkSubpassDependency> vulkan_subpass_dependencies() const;

   Device& m_device;
   SampleCount m_sampleCount = SampleCount::Single;
   std::vector<AttachmentInfo> m_attachments;
};


}// namespace triglav::graphics_api