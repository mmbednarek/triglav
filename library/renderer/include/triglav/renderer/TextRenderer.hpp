#pragma once

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Array.hpp"
#include "triglav/graphics_api/CommandList.hpp"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/render_core/GlyphAtlas.h"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.h"

#include "GlyphCache.hpp"

#include <glm/vec2.hpp>

namespace triglav::renderer {

struct TextObject
{
   const render_core::GlyphAtlas* glyphAtlas;
   render_core::TextMetric metric;
   graphics_api::UniformBuffer<triglav::render_core::SpriteUBO> ubo;
   graphics_api::VertexArray<render_core::GlyphVertex> vertices;
   u32 vertexCount;
};

class TextRenderer
{
 public:
   explicit TextRenderer(graphics_api::Device& device, resource::ResourceManager& resourceManager, graphics_api::RenderTarget& renderTarget,
                         GlyphCache& glyphCache);

   void bind_pipeline(graphics_api::CommandList& cmdList);
   TextObject create_text_object(TypefaceName typefaceName, int fontSize, std::string_view content);
   void draw_text(graphics_api::CommandList& cmdList, const TextObject& textObject, const glm::vec2& viewportSize,
                  const glm::vec2& position, const glm::vec4& color) const;
   void update_text(TextObject& textObject, std::string_view content);

   [[nodiscard]] const graphics_api::Pipeline& pipeline() const;

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   GlyphCache& m_glyphCache;
   graphics_api::Pipeline m_pipeline;
};

}// namespace triglav::renderer
