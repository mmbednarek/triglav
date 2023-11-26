#include "RenderPass.h"

#include <limits>

namespace graphics_api {

RenderPass::RenderPass(vulkan::RenderPass renderPass, std::vector<Texture> textures,
                       std::vector<AttachmentType> attachmentLayout, const Resolution &resolution,
                       const ColorFormat &colorFormat, const SampleCount sampleCount) :
    m_renderPass(std::move(renderPass)),
    m_textures(std::move(textures)),
    m_attachmentLayout(std::move(attachmentLayout)),
    m_resolution(resolution),
    m_colorFormat(colorFormat),
    m_sampleCount(sampleCount)
{
}

VkRenderPass RenderPass::vulkan_render_pass() const
{
   return *m_renderPass;
}

std::vector<VkImageView> RenderPass::image_views() const
{
   std::vector<VkImageView> result{};
   result.resize(m_attachmentLayout.size());
   size_t j = 0;

   for (size_t i = 0; i < m_attachmentLayout.size(); ++i) {
      if (m_attachmentLayout[i] == AttachmentType::ResolveAttachment) {
         continue;
      }
      result[i] = m_textures[j].vulkan_image_view();
      ++j;
   }

   return result;
}

size_t RenderPass::resolve_attachment_id() const
{
   size_t i = 0;
   for (const auto &attachment : m_attachmentLayout) {
      if (attachment == AttachmentType::ResolveAttachment)
         return i;
      ++i;
   }
   return std::numeric_limits<size_t>::max();
}

Resolution RenderPass::resolution() const
{
   return m_resolution;
}

ColorFormat RenderPass::color_format() const
{
   return m_colorFormat;
}

SampleCount RenderPass::sample_count() const
{
   return m_sampleCount;
}

}// namespace graphics_api