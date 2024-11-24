#pragma once

#include "Geometry.hpp"

#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/renderer/BindlessScene.hpp"
#include "triglav/resource/ResourceManager.h"

#include <glm/mat4x4.hpp>
#include <triglav/graphics_api/HostVisibleBuffer.hpp>

namespace triglav::renderer::node {

struct UniformViewProperties
{
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
   float nearPlane;
   float farPlane;
};

class BindlessGeometryResources final : public IGeometryResources
{
   friend class BindlessGeometry;

 public:
   explicit BindlessGeometryResources(graphics_api::Device& device);
   graphics_api::UniformBuffer<UniformViewProperties>& view_properties();

   [[nodiscard]] GroundRenderer::UniformBuffer& ground_ubo() override;

 private:
   graphics_api::UniformBuffer<UniformViewProperties> m_uniformBuffer;
   GroundRenderer::UniformBuffer m_groundUniformBuffer;
   graphics_api::StorageArray<BindlessSceneObject> m_visibleObjects;
   graphics_api::Buffer m_countBuffer;
   graphics_api::Texture m_hiZBuffer;
   graphics_api::Texture m_hiZBufferScratch;
};

class BindlessGeometry final : public render_core::IRenderNode
{
 public:
   BindlessGeometry(graphics_api::Device& device, BindlessScene& bindlessScene, resource::ResourceManager& resourceManager);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;

   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& nodeResources,
                        graphics_api::CommandList& cmdList) override;

   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;

 private:
   graphics_api::Device& m_device;
   BindlessScene& m_bindlessScene;
   graphics_api::RenderTarget m_renderTarget;
   graphics_api::RenderTarget m_depthPrepassRenderTarget;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Pipeline m_depthPrepassPipeline;
   graphics_api::Pipeline m_hiZBufferPipeline;
   graphics_api::Pipeline m_cullingPipeline;
};

}// namespace triglav::renderer::node
