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
                  resource::ResourceManager &resourceManager);

   [[nodiscard]] render_core::Sprite create_sprite(triglav::Name textureName);
   [[nodiscard]] render_core::Sprite
   create_sprite_from_texture(const graphics_api::Texture &texture);

   void update_resolution(const graphics_api::Resolution &resolution);
   void begin_render(graphics_api::CommandList& cmdList) const;
   void draw_sprite(graphics_api::CommandList& cmdList, const render_core::Sprite &sprite, glm::vec2 position, glm::vec2 scale) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::RenderPass &m_renderPass;
   resource::ResourceManager &m_resourceManager;
   graphics_api::Resolution m_resolution{};

   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler& m_sampler;
};

}// namespace triglav::renderer