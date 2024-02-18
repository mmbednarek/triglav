#pragma once

#include "graphics_api/Device.h"
#include "graphics_api/Pipeline.h"
#include "triglav/render_core/Model.hpp"
#include "triglav/resource/ResourceManager.h"

#include "GlyphAtlas.h"

namespace renderer {

struct TextObject
{
   std::string text;
   TextMetric metric;
   graphics_api::DescriptorArray descriptors;
   graphics_api::UniformBuffer<triglav::render_core::SpriteUBO> ubo;
   graphics_api::VertexArray<GlyphVertex> vertices;
   int vertexCount;
};

struct TextColorConstant
{
   glm::vec3 color;
};

class TextRenderer
{
 public:
   TextRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                triglav::resource::ResourceManager &resourceManager);

   void begin_render(graphics_api::CommandList &cmdList) const;
   TextObject create_text_object(const GlyphAtlas &atlas, std::string_view text);
   void update_text_object(const GlyphAtlas &atlas, TextObject &object, std::string_view text) const;
   void update_resolution(const graphics_api::Resolution &resolution);
   void draw_text_object(const graphics_api::CommandList &cmdList, const TextObject &textObj,
                         glm::vec2 position, glm::vec3 color) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::RenderPass &m_renderPass;
   triglav::resource::ResourceManager &m_resourceManager;
   graphics_api::Resolution m_resolution;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler m_sampler;
};


}// namespace renderer