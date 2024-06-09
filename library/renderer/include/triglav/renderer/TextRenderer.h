#pragma once

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Array.hpp"
#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/render_core/GlyphAtlas.h"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.h"

#include <glm/vec2.hpp>

namespace triglav::renderer {

struct TextObject
{
   GlyphAtlasName glyphAtlas;
   render_core::TextMetric metric;
   graphics_api::UniformBuffer<triglav::render_core::SpriteUBO> ubo;
   graphics_api::VertexArray<render_core::GlyphVertex> vertices;
   u32 vertexCount;
};

class TextRenderer
{
 public:
   explicit TextRenderer(graphics_api::Device& device, resource::ResourceManager& resourceManager,
                         graphics_api::RenderTarget& renderTarget);

   void bind_pipeline(graphics_api::CommandList& cmdList);
   TextObject create_text_object(GlyphAtlasName atlasName, std::string_view content);
   void draw_text(graphics_api::CommandList& cmdList, const TextObject& textObject, const glm::vec2& viewportSize,
                  const glm::vec2& position, const glm::vec4& color) const;
   void update_text(TextObject& textObject, std::string_view content);

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   graphics_api::Pipeline m_pipeline;
};

}// namespace triglav::renderer
