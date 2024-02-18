#pragma once

#include "graphics_api/DepthRenderTarget.h"
#include "graphics_api/Pipeline.h"
#include "graphics_api/RenderPass.h"
#include "triglav/render_core/Model.hpp"

namespace triglav::resource {
class ResourceManager;
}

namespace renderer {

class ModelRenderer;

class ShadowMap
{
 public:
   ShadowMap(graphics_api::Device &device, triglav::resource::ResourceManager &resourceManager);

   [[nodiscard]] const graphics_api::Texture &depth_texture() const;
   [[nodiscard]] const graphics_api::Framebuffer &framebuffer() const;

   void on_begin_render(const ModelRenderer &ctx) const;
   void draw_model(const ModelRenderer &ctx,
                   const triglav::render_core::InstancedModel &instancedModel) const;
   triglav::render_core::ModelShaderMapProperties create_model_properties();

 private:
   graphics_api::Device &m_device;
   triglav::resource::ResourceManager &m_resourceManager;

   graphics_api::DepthRenderTarget m_depthRenderTarget;
   graphics_api::Texture m_depthTexture;
   graphics_api::RenderPass m_renderPass;
   graphics_api::Framebuffer m_framebuffer;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
};

}// namespace renderer