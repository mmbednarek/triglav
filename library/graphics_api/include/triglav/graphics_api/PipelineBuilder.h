#pragma once

#include "GraphicsApi.hpp"
#include "Pipeline.h"

#include <vector>

namespace triglav::graphics_api {

class Device;
class RenderTarget;
class Shader;

class PipelineBuilder
{
 public:
   PipelineBuilder(Device &device, RenderTarget &renderPass);

   PipelineBuilder &fragment_shader(const Shader &shader);
   PipelineBuilder &vertex_shader(const Shader &shader);
   PipelineBuilder &vertex_attribute(const ColorFormat &format, size_t offset);
   PipelineBuilder &end_vertex_layout();
   PipelineBuilder &descriptor_binding(DescriptorType descriptorType, PipelineStage shaderStage);
   PipelineBuilder &push_constant(PipelineStage shaderStage, size_t size, size_t offset = 0);
   PipelineBuilder &enable_depth_test(bool enabled);
   PipelineBuilder &enable_blending(bool enabled);
   PipelineBuilder &use_push_descriptors(bool enabled);
   PipelineBuilder &vertex_topology(VertexTopology topology);
   PipelineBuilder &razterization_method(RasterizationMethod method);
   PipelineBuilder &culling(Culling cull);

   [[nodiscard]] Result<Pipeline> build() const;

   template<typename TVertex>
   PipelineBuilder &begin_vertex_layout()
   {
      VkVertexInputBindingDescription binding{};
      binding.binding   = m_vertexBinding;
      binding.stride    = sizeof(TVertex);
      binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      m_bindings.emplace_back(binding);
      return *this;
   }

 private:
   Device &m_device;
   RenderTarget &m_renderTarget;

   uint32_t m_vertexLocation{0};
   uint32_t m_vertexBinding{0};
   VkPolygonMode m_polygonMode             = VK_POLYGON_MODE_FILL;
   VkPrimitiveTopology m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   VkFrontFace m_frontFace                 = VK_FRONT_FACE_CLOCKWISE;
   bool m_depthTestEnabled{false};
   bool m_blendingEnabled{true};
   bool m_usePushDescriptors{false};

   std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
   std::vector<VkVertexInputBindingDescription> m_bindings{};
   std::vector<VkVertexInputAttributeDescription> m_attributes{};
   std::vector<VkDescriptorSetLayoutBinding> m_vulkanDescriptorBindings{};
   std::vector<VkPushConstantRange> m_pushConstantRanges{};
};

}// namespace triglav::graphics_api