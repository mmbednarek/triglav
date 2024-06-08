#pragma once

#include "triglav/graphics_api/RenderTarget.h"
#include "triglav/render_core/FrameResources.h"
#include "triglav/render_core/IRenderNode.hpp"

#include <optional>

namespace triglav::renderer::node {

class DownsampleResources : public render_core::NodeFrameResources
{
 public:
   explicit DownsampleResources(graphics_api::Device& device);

   void update_resolution(const graphics_api::Resolution& resolution) override;

   [[nodiscard]] graphics_api::Texture& texture();

 private:
   graphics_api::Device& m_device;
   std::optional<graphics_api::Texture> m_downsampledTexture;
};

class Downsample : public render_core::IRenderNode
{
 public:
   Downsample(graphics_api::Device& device, Name srcNode, Name srcFramebuffer, Name srcAttachment);

   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;
   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;

 private:
   graphics_api::Device& m_device;
   Name m_srcNode;
   Name m_srcFramebuffer;
   Name m_srcAttachment;
};

}// namespace triglav::renderer::node
