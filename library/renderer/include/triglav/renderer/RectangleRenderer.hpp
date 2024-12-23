#pragma once

#include "triglav/graphics_api/DescriptorPool.hpp"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::renderer {

struct Rectangle
{
   struct VertexUBO
   {
      glm::vec2 position{};
      glm::vec2 viewportSize;
   };

   struct FragmentUBO
   {
      glm::vec4 borderRadius{10.0f, 10.0f, 10.0f, 10.0f};
      glm::vec4 backgroundColor{0.0f, 0.0f, 0.0f, 0.8f};
      glm::vec2 rectSize{100, 100};
   };

   glm::vec4 rect{};
   graphics_api::VertexArray<glm::vec2> array;
   graphics_api::UniformBuffer<VertexUBO> vertexUBO;
   graphics_api::UniformBuffer<FragmentUBO> fragmentUBO;
   graphics_api::DescriptorArray descriptors;
};

class RectangleRenderer
{
 public:
   RectangleRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget,
                     triglav::resource::ResourceManager& resourceManager);

   [[nodiscard]] Rectangle create_rectangle(glm::vec4 rect, glm::vec4 backgroundColor);
   void update_rectangle(Rectangle& rectangle, glm::vec4 rectCoords, glm::vec4 backgroundColor);
   void begin_render(graphics_api::CommandList& cmdList) const;
   void draw(graphics_api::CommandList& cmdList, const Rectangle& rect, const graphics_api::Resolution& resolution) const;

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
};

}// namespace triglav::renderer