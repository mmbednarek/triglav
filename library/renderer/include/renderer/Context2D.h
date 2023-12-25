#pragma once

#include "graphics_api/Device.h"

#include "ResourceManager.h"

namespace renderer {

class Context2D
{
 public:
   Context2D(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
             ResourceManager &resourceManager);

   void set_active_command_list(graphics_api::CommandList *commandList);
   void begin_render();
   void draw_sprite(const Sprite& sprite, glm::vec2 position) const;
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