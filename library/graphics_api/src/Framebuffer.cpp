#include "Framebuffer.h"

namespace triglav::graphics_api {

Framebuffer::Framebuffer(const Resolution resolution, const VkRenderPass renderPass,
                         vulkan::Framebuffer framebuffer) :
    m_resolution(resolution),
    m_renderPass(renderPass),
    m_framebuffer(std::move(framebuffer))
{
}

Resolution Framebuffer::resolution() const
{
   return m_resolution;
}

VkFramebuffer Framebuffer::vulkan_framebuffer() const
{
   return *m_framebuffer;
}

VkRenderPass Framebuffer::vulkan_render_pass() const
{
   return m_renderPass;
}

}// namespace graphics_api
