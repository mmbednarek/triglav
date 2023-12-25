#include "RenderPass.h"

namespace graphics_api {

RenderPass::RenderPass(vulkan::RenderPass renderPass, const Resolution &resolution,
                       const SampleCount sampleCount) :
    m_renderPass(std::move(renderPass)),
    m_resolution(resolution),
    m_sampleCount(sampleCount)
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

}// namespace graphics_api