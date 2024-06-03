#pragma once

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/graphics_api/Texture.h"
#include "triglav/render_core/FrameResources.h"

#include "triglav/resource/ResourceManager.h"

namespace triglav::renderer {

class PostProcessingRenderer
{
 public:
   struct PushConstants
   {
      int enableFXAA{};
      int hideUI{};
      int bloomEnabled{};
   };

   PostProcessingRenderer(graphics_api::Device &device, graphics_api::RenderTarget &renderTarget,
                          resource::ResourceManager &resourceManager);

   void draw(render_core::FrameResources &resources, graphics_api::CommandList &cmdList) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Sampler &m_sampler;
};

}// namespace triglav::renderer