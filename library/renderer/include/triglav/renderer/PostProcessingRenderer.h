#pragma once

#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/graphics_api/Texture.hpp"
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

   PostProcessingRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget,
                          resource::ResourceManager& resourceManager);

   void draw(render_core::FrameResources& resources, graphics_api::CommandList& cmdList) const;

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
};

}// namespace triglav::renderer