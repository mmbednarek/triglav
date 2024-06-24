#pragma once

#include "GlyphCache.h"

#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/resource/ResourceManager.h"

namespace triglav::renderer::node {

class ProcessGlyphsResources : public render_core::NodeFrameResources
{
 public:
   ProcessGlyphsResources(graphics_api::Device& device);

   [[nodiscard]] graphics_api::Buffer& vertex_buffer();

 private:
   graphics_api::Buffer m_vertexBuffer;
};

   class ProcessGlyphs : public render_core::IRenderNode
{
 public:
   ProcessGlyphs(graphics_api::Device& device, resource::ResourceManager& resourceManager, GlyphCache& glyphCache);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;

 private:
   graphics_api::Device& m_device;
   GlyphCache& m_glyphCache;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Buffer m_textBuffer;
};

}
