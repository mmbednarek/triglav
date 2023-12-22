#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

namespace graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(Framebuffer, Device)

class Framebuffer
{
 public:
   Framebuffer(Resolution resolution, VkRenderPass renderPass, vulkan::Framebuffer framebuffer);

   [[nodiscard]] Resolution resolution() const;
   [[nodiscard]] VkFramebuffer vulkan_framebuffer() const;
   [[nodiscard]] VkRenderPass vulkan_render_pass() const;

 private:
   Resolution m_resolution;
   VkRenderPass m_renderPass;
   vulkan::Framebuffer m_framebuffer;
};

}// namespace graphics_api
