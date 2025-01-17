#pragma once

#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/renderer/Scene.hpp"

#include "RayTracingScene.hpp"

namespace triglav::renderer::node {

class RayTracedImageResources : public render_core::NodeFrameResources
{
 public:
   explicit RayTracedImageResources(graphics_api::Device& device);

   [[nodiscard]] const graphics_api::Texture& ao_texture() const;
   [[nodiscard]] const graphics_api::Texture& shadows_texture() const;
   [[nodiscard]] graphics_api::TextureState in_texture_state();
   void update_resolution(const graphics_api::Resolution& resolution) override;

 private:
   graphics_api::Device& m_device;
   graphics_api::Texture m_ambientOcclusionTexture;
   graphics_api::Texture m_shadowsTexture;
   bool m_initialLayout{true};
};

class RayTracedImage : public render_core::IRenderNode
{
 public:
   RayTracedImage(graphics_api::Device& device, RayTracingScene& rtScene);
   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& nodeResources,
                        graphics_api::CommandList& cmdList) override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;
   [[nodiscard]] float gpu_time() const;

 private:
   graphics_api::Device& m_device;
   RayTracingScene& m_rtScene;
   graphics_api::TimestampArray m_timestampArray;
};

}// namespace triglav::renderer::node