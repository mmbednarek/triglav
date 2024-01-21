#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <vector>

namespace graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(RenderPass, Device);

class RenderPass
{
 public:
   RenderPass(vulkan::RenderPass renderPass, const Resolution &resolution, SampleCount sampleCount,
              int colorAttachmentCount);

   [[nodiscard]] VkRenderPass vulkan_render_pass() const;
   [[nodiscard]] Resolution resolution() const;
   [[nodiscard]] SampleCount sample_count() const;
   [[nodiscard]] int color_attachment_count() const;

 private:
   vulkan::RenderPass m_renderPass;
   Resolution m_resolution;
   SampleCount m_sampleCount;
   int m_colorAttachmentCount;
};

}// namespace graphics_api