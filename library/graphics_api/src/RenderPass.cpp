#include "RenderPass.h"

namespace graphics_api {

RenderPass::RenderPass(vulkan::RenderPass renderPass, const Resolution &resolution,
                       const SampleCount sampleCount, const int colorAttachmentCount) :
    m_renderPass(std::move(renderPass)),
    m_resolution(resolution),
    m_sampleCount(sampleCount),
    m_colorAttachmentCount(colorAttachmentCount)
{
}

VkRenderPass RenderPass::vulkan_render_pass() const
{
   return *m_renderPass;
}

Resolution RenderPass::resolution() const
{
   return m_resolution;
}

SampleCount RenderPass::sample_count() const
{
   return m_sampleCount;
}

int RenderPass::color_attachment_count() const
{
   return m_colorAttachmentCount;
}

}// namespace graphics_api