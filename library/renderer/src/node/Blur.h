#pragma once

#include <optional>

#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/resource/ResourceManager.h"

namespace triglav::renderer::node {

class BlurResources : public render_core::NodeFrameResources
{
 public:
   explicit BlurResources(graphics_api::Device& device, bool isSingleChannel);

   void update_resolution(const graphics_api::Resolution& resolution) override;

   [[nodiscard]] graphics_api::Texture& texture();
   [[nodiscard]] graphics_api::TextureState texture_state();

 private:
   graphics_api::Device& m_device;
   std::optional<graphics_api::Texture> m_outputTexture;
   bool m_initialLayout{true};
   bool m_isSingleChannel{};
};

class Blur : public render_core::IRenderNode
{
 public:
   Blur(graphics_api::Device& device, resource::ResourceManager& resourceManager, Name srcNode, Name srcFramebuffer, Name srcAttachment,
        bool isSingleChannel);

   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;
   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
   Name m_srcNode;
   Name m_srcFramebuffer;
   Name m_srcAttachment;
   bool m_isSingleChannel;
};

}// namespace triglav::renderer::node