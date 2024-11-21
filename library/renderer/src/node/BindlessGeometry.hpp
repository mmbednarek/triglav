#pragma once

#include "triglav/geometry/Geometry.h"
#include "triglav/graphics_api/Array.hpp"
#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/resource/ResourceManager.h"

#include <glm/mat4x4.hpp>
#include <triglav/graphics_api/HostVisibleBuffer.hpp>

namespace triglav::renderer {
class BindlessScene;
}

namespace triglav::renderer::node {

struct UniformViewProperties
{
   glm::mat4 view;
   glm::mat4 proj;
   glm::mat4 normal;
};

class BindlessGeometryResources final : public render_core::NodeFrameResources
{
 public:
   explicit BindlessGeometryResources(graphics_api::Device& device);

   graphics_api::UniformBuffer<UniformViewProperties>& view_properties();

 private:
   graphics_api::UniformBuffer<UniformViewProperties> m_uniformBuffer;
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
   graphics_api::Pipeline m_pipeline;
};

}// namespace triglav::renderer::node
