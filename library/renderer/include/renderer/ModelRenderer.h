#pragma once

#include "graphics_api/Pipeline.h"
#include "graphics_api/DescriptorPool.h"

#include "Model.h"

namespace graphics_api {
class Device;
class RenderPass;
class Pipeline;
}// namespace graphics_api

namespace renderer {

class ResourceManager;
class ShadowMap;

class ModelRenderer
{
 public:
   ModelRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                 ResourceManager &resourceManager);

   InstancedModel instance_model(Name modelName, ShadowMap &shadowMap);
   void set_active_command_list(graphics_api::CommandList *commandList);
   void begin_render();
   void draw_model(const InstancedModel &instancedModel) const;
   [[nodiscard]] graphics_api::CommandList &command_list() const;

 private:
   graphics_api::Device &m_device;
   ResourceManager &m_resourceManager;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler m_sampler;

   graphics_api::CommandList *m_commandList{};
};

}// namespace renderer