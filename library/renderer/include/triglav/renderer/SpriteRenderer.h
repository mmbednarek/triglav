#pragma once

#include "triglav/graphics_api/DescriptorPool.h"
#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/graphics_api/Sampler.h"
#include "triglav/render_core/Model.hpp"
#include "triglav/resource/ResourceManager.h"

namespace triglav::renderer {

class SpriteRenderer
{
 public:
   SpriteRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                  triglav::resource::ResourceManager &resourceManager);

   [[nodiscard]] triglav::render_core::Sprite create_sprite(triglav::Name textureName);
   [[nodiscard]] triglav::render_core::Sprite
   create_sprite_from_texture(const graphics_api::Texture &texture);

   void update_resolution(const graphics_api::Resolution &resolution);
   void set_active_command_list(graphics_api::CommandList *commandList);
   void begin_render() const;
   void draw_sprite(const triglav::render_core::Sprite &sprite, glm::vec2 position, glm::vec2 scale) const;
   [[nodiscard]] graphics_api::CommandList &command_list() const;

 private:
   graphics_api::Device &m_device;
   graphics_api::RenderPass &m_renderPass;
   triglav::resource::ResourceManager &m_resourceManager;
   graphics_api::Resolution m_resolution{};

   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler& m_sampler;

   graphics_api::CommandList *m_commandList{};
};

}// namespace triglav::renderer