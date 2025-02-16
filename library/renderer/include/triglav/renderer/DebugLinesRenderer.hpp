#pragma once

#include "triglav/graphics_api/Array.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/render_objects/Mesh.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include "Camera.hpp"

namespace triglav::renderer {

struct DebugLines
{
   graphics_api::VertexArray<glm::vec3> array;
   glm::mat4 model;
   graphics_api::UniformBuffer<glm::mat4> ubo;
   graphics_api::DescriptorArray descriptors;
};

class DebugLinesRenderer
{
 public:
   DebugLinesRenderer(graphics_api::Device& device, resource::ResourceManager& resourceManager);

   [[nodiscard]] DebugLines create_line_list(std::span<glm::vec3> list);
   [[nodiscard]] DebugLines create_line_list_from_bounding_box(const geometry::BoundingBox& boundingBox);
   void begin_render(graphics_api::CommandList& cmdList) const;
   void draw(graphics_api::CommandList& cmdList, const DebugLines& list, const Camera& camera) const;

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
};

}// namespace triglav::renderer