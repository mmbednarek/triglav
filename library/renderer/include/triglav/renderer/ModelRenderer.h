#pragma once

#include "triglav/graphics_api/Pipeline.h"
#include "triglav/graphics_api/DescriptorPool.h"
#include "triglav/Name.hpp"
#include "triglav/render_core/Model.hpp"

namespace triglav::graphics_api {
class Device;
class RenderPass;
class Pipeline;
}// namespace graphics_api

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::renderer {

class ShadowMapRenderer;

class ModelRenderer
{
 public:
   ModelRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                 resource::ResourceManager &resourceManager);

   render_core::InstancedModel instance_model(Name modelName, ShadowMapRenderer &shadowMap);
   void begin_render(graphics_api::CommandList& cmdList) const;
   void draw_model(const graphics_api::CommandList& cmdList, const render_core::InstancedModel &instancedModel) const;

 private:
   graphics_api::Device &m_device;
   resource::ResourceManager &m_resourceManager;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler& m_sampler;
};

}// namespace renderer