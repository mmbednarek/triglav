#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include "triglav/Name.hpp"

#include <map>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(Framebuffer, Device)

class Framebuffer
{
 public:
   Framebuffer(Resolution resolution, VkRenderPass renderPass, vulkan::Framebuffer framebuffer, std::map<Name, Texture> textures);

   [[nodiscard]] Resolution resolution() const;
   [[nodiscard]] VkFramebuffer vulkan_framebuffer() const;
   [[nodiscard]] VkRenderPass vulkan_render_pass() const;

   [[nodiscard]] Texture& texture(Name index);

 private:
   Resolution m_resolution;
   VkRenderPass m_renderPass;
   vulkan::Framebuffer m_framebuffer;
   std::map<Name, Texture> m_textures;
};

}// namespace triglav::graphics_api
