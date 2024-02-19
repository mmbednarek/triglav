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

class ShadowMap;

class ModelRenderer
{
 public:
   ModelRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                 triglav::resource::ResourceManager &resourceManager);

   triglav::render_core::InstancedModel instance_model(triglav::Name modelName, ShadowMap &shadowMap);
   void set_active_command_list(graphics_api::CommandList *commandList);
   void begin_render();
   void draw_model(const triglav::render_core::InstancedModel &instancedModel) const;
   [[nodiscard]] graphics_api::CommandList &command_list() const;

 private:
   graphics_api::Device &m_device;
   triglav::resource::ResourceManager &m_resourceManager;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler m_sampler;

   graphics_api::CommandList *m_commandList{};
};

}// namespace renderer