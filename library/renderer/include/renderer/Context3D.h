#pragma once

#include "Model.h"

#include <glm/vec3.hpp>

namespace graphics_api {
class Device;
class RenderPass;
class Pipeline;
}// namespace graphics_api

namespace renderer {

class ResourceManager;

struct PushConstant
{
   glm::vec3 lightPosition{};
};

class Context3D
{
 public:
   Context3D(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
             ResourceManager &resourceManager);

   InstancedModel instance_model(Name modelName);
   void begin_render(graphics_api::CommandList *commandList);
   void draw_model(const InstancedModel &instancedModel) const;
   void set_light_position(glm::vec3 pos);

 private:
   graphics_api::Device &m_device;
   graphics_api::RenderPass &m_renderPass;
   ResourceManager &m_resourceManager;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler m_sampler;
   PushConstant m_pushConstant;

   graphics_api::CommandList *m_commandList{};
};

}// namespace renderer