#pragma once

#include "Core.h"
#include "ResourceManager.h"

#include "graphics_api/DescriptorPool.h"
#include "graphics_api/Pipeline.h"

namespace renderer {

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
      glm::vec4 backgroundColor{0.0f, 0.0f, 0.0f, 0.6f};
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
   RectangleRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                     const ResourceManager &resourceManager);

   [[nodiscard]] Rectangle create_rectangle(glm::vec4 rect);
   void begin_render(graphics_api::CommandList &cmdList) const;
   void draw(const graphics_api::CommandList &cmdList, const graphics_api::RenderPass &renderPass,
             const Rectangle &rect, const graphics_api::Resolution& resolution) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
};

}// namespace renderer