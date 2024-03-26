#pragma once

#include "triglav/graphics_api/RenderTarget.h"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/render_core/Model.hpp"

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::renderer {

class ModelRenderer;

class ShadowMapRenderer
{
 public:
   ShadowMapRenderer(graphics_api::Device &device, resource::ResourceManager &resourceManager);

   [[nodiscard]] const graphics_api::Texture &depth_texture();
   [[nodiscard]] const graphics_api::Framebuffer &framebuffer() const;

   void on_begin_render(graphics_api::CommandList &cmdList) const;
   void draw_model(graphics_api::CommandList &cmdList,
                   const render_core::InstancedModel &instancedModel) const;
   render_core::ModelShaderMapProperties create_model_properties();

 private:
   graphics_api::Device &m_device;
   resource::ResourceManager &m_resourceManager;

   graphics_api::RenderTarget m_depthRenderTarget;
   graphics_api::Framebuffer m_framebuffer;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
};

}// namespace renderer