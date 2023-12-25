#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <vector>

namespace graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(RenderPass, Device);

class RenderPass
{
 public:
   RenderPass(vulkan::RenderPass renderPass, const Resolution &resolution, SampleCount sampleCount);

   [[nodiscard]] VkRenderPass vulkan_render_pass() const;
   [[nodiscard]] Resolution resolution() const;
   [[nodiscard]] SampleCount sample_count() const;

 private:
   vulkan::RenderPass m_renderPass;
   Resolution m_resolution;
   SampleCount m_sampleCount;
};

}// namespace graphics_api