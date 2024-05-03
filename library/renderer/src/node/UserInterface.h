#pragma once

#include "RectangleRenderer.h"

#include "triglav/Delegate.hpp"
#include "triglav/graphics_api/RenderTarget.h"
#include "triglav/graphics_api/Texture.h"
#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/ui_core/Viewport.h"

#include <map>
#include <string_view>

namespace triglav::renderer::node {

class UserInterface : public render_core::IRenderNode
{
 public:
   using OnAddedLabelGroupDel = Delegate<NameID, std::string_view>;
   using OnAddedLabelDel = Delegate<NameID, NameID, std::string_view>;

   OnAddedLabelGroupDel OnAddedLabelGroup;
   OnAddedLabelDel OnAddedLabel;

   UserInterface(graphics_api::Device &device, resource::ResourceManager &resourceManager, ui_core::Viewport &viewport);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;
   void record_commands(render_core::FrameResources &frameResources,
                        render_core::NodeFrameResources &resources,
                        graphics_api::CommandList &cmdList) override;

 private:
   graphics_api::Device &m_device;
   resource::ResourceManager &m_resourceManager;
   ui_core::Viewport &m_viewport;
   graphics_api::RenderTarget m_textureRenderTarget;
   graphics_api::Pipeline m_textPipeline;
   RectangleRenderer m_rectangleRenderer;
};

}// namespace triglav::renderer::node
