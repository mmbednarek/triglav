#pragma once

#include "GlyphCache.h"
#include "TextRenderer.h"

#include "triglav/Delegate.hpp"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.h"
#include "triglav/ui_core/Viewport.h"

namespace triglav::renderer::node {

struct PendingTextUpdate
{
   Name name;
   const ui_core::Text* textInfo;
   u32 textOffset;
   u32 textCount;
};

struct TextResources
{
   const render_core::GlyphAtlas* atlas;
   const ui_core::Text* textInfo;

   graphics_api::Buffer vertexBuffer;
   graphics_api::UniformBuffer<render_core::SpriteUBO> ubo;

   u32 vertexCount;

   TextResources(graphics_api::Device& device, const render_core::GlyphAtlas* atlas, const ui_core::Text* textInfo, u32 vertexCount);
   void update_size(graphics_api::Device& device, u32 vertexCount);
};

class ProcessGlyphsResources : public render_core::NodeFrameResources
{
 public:
   using Self = ProcessGlyphsResources;

   ProcessGlyphsResources(graphics_api::Device& device, graphics_api::Pipeline& pipeline, GlyphCache& glyphCache,
                          ui_core::Viewport& viewport);

   void on_added_text(Name name, const ui_core::Text& text);
   void update_resources(graphics_api::CommandList& cmdList);
   void draw(graphics_api::CommandList& cmdList, const graphics_api::Pipeline& textPipeline, const glm::vec2& viewportSize);

 private:
   static void draw_text(graphics_api::CommandList& cmdList, const TextResources& resources, const glm::vec2& viewportSize);

   graphics_api::Device& m_device;
   graphics_api::Pipeline& m_pipeline;
   GlyphCache& m_glyphCache;
   graphics_api::Buffer m_textStagingBuffer;
   graphics_api::Buffer m_textBuffer;
   std::vector<PendingTextUpdate> m_pendingUpdates;
   std::map<Name, TextResources> m_textResources;

   ui_core::Viewport::OnAddedTextDel::Sink<Self> m_onAddedTextSink;
   ui_core::Viewport::OnTextChangeContentDel::Sink<Self> m_onTextChangeContentSink;
};

class ProcessGlyphs : public render_core::IRenderNode
{
 public:
   ProcessGlyphs(graphics_api::Device& device, resource::ResourceManager& resourceManager, GlyphCache& glyphCache,
                 ui_core::Viewport& viewport);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;

 private:
   graphics_api::Device& m_device;
   GlyphCache& m_glyphCache;
   ui_core::Viewport& m_viewport;
   graphics_api::Pipeline m_pipeline;
};

}// namespace triglav::renderer::node
