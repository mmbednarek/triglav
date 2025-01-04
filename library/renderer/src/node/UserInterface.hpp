#pragma once

#include "RectangleRenderer.hpp"

#include "triglav/Delegate.hpp"
#include "triglav/graphics_api/RenderTarget.hpp"
#include "triglav/graphics_api/Texture.hpp"
#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/renderer/TextRenderer.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <map>
#include <string_view>

namespace triglav::renderer::node {

class UserInterface : public render_core::IRenderNode
{
 public:
   using OnAddedLabelGroupDel = Delegate<Name, std::string_view>;
   using OnAddedLabelDel = Delegate<Name, Name, std::string_view>;

   OnAddedLabelGroupDel OnAddedLabelGroup;
   OnAddedLabelDel OnAddedLabel;

   UserInterface(graphics_api::Device& device, resource::ResourceManager& resourceManager, ui_core::Viewport& viewport,
                 GlyphCache& glyphCache);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   ui_core::Viewport& m_viewport;
   graphics_api::RenderTarget m_textureRenderTarget;
   TextRenderer m_textRenderer;
   RectangleRenderer m_rectangleRenderer;
   graphics_api::UniformBuffer<render_objects::SpriteUBO> m_ubo;
};

}// namespace triglav::renderer::node
