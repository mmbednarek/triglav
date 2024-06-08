#pragma once

#include "GraphicsApi.hpp"
#include "Pipeline.h"

#include <vector>

namespace triglav::graphics_api {

class Device;
class RenderTarget;
class Shader;

class PipelineBuilderBase
{
 public:
   explicit PipelineBuilderBase(Device& device);

 protected:
   void add_shader(const Shader& shader);
   void add_descriptor_binding(DescriptorType descriptorType, PipelineStage shaderStage);
   void add_push_constant(PipelineStage shaderStage, size_t size, size_t offset = 0);

   [[nodiscard]] Result<std::tuple<vulkan::DescriptorSetLayout, vulkan::PipelineLayout>> build_pipeline_layout() const;

   Device& m_device;
   std::vector<VkDescriptorSetLayoutBinding> m_vulkanDescriptorBindings{};
   std::vector<VkPushConstantRange> m_pushConstantRanges{};
   std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
   bool m_usePushDescriptors{false};
};

class ComputePipelineBuilder : public PipelineBuilderBase
{
 public:
   explicit ComputePipelineBuilder(Device& device);

   ComputePipelineBuilder& compute_shader(const Shader& shader);
   ComputePipelineBuilder& descriptor_binding(DescriptorType descriptorType);
   ComputePipelineBuilder& push_constant(PipelineStage shaderStage, size_t size, size_t offset = 0);
   ComputePipelineBuilder& use_push_descriptors(bool enabled);

   [[nodiscard]] Result<Pipeline> build() const;
};

class GraphicsPipelineBuilder : public PipelineBuilderBase
{
 public:
   GraphicsPipelineBuilder(Device& device, RenderTarget& renderPass);

   GraphicsPipelineBuilder& fragment_shader(const Shader& shader);
   GraphicsPipelineBuilder& vertex_shader(const Shader& shader);
   GraphicsPipelineBuilder& vertex_attribute(const ColorFormat& format, size_t offset);
   GraphicsPipelineBuilder& end_vertex_layout();
   GraphicsPipelineBuilder& descriptor_binding(DescriptorType descriptorType, PipelineStage shaderStage);
   GraphicsPipelineBuilder& push_constant(PipelineStage shaderStage, size_t size, size_t offset = 0);
   GraphicsPipelineBuilder& enable_depth_test(bool enabled);
   GraphicsPipelineBuilder& depth_test_mode(DepthTestMode mode);
   GraphicsPipelineBuilder& enable_blending(bool enabled);
   GraphicsPipelineBuilder& use_push_descriptors(bool enabled);
   GraphicsPipelineBuilder& vertex_topology(VertexTopology topology);
   GraphicsPipelineBuilder& rasterization_method(RasterizationMethod method);
   GraphicsPipelineBuilder& culling(Culling cull);

   [[nodiscard]] Result<Pipeline> build() const;

   template<typename TVertex>
   GraphicsPipelineBuilder& begin_vertex_layout()
   {
      VkVertexInputBindingDescription binding{};
      binding.binding = m_vertexBinding;
      binding.stride = sizeof(TVertex);
      binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      m_bindings.emplace_back(binding);
      return *this;
   }

 private:
   RenderTarget& m_renderTarget;

   uint32_t m_vertexLocation{0};
   uint32_t m_vertexBinding{0};
   VkPolygonMode m_polygonMode = VK_POLYGON_MODE_FILL;
   VkPrimitiveTopology m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   VkFrontFace m_frontFace = VK_FRONT_FACE_CLOCKWISE;
   DepthTestMode m_depthTestMode{DepthTestMode::Disabled};
   bool m_blendingEnabled{true};

   std::vector<VkVertexInputBindingDescription> m_bindings{};
   std::vector<VkVertexInputAttributeDescription> m_attributes{};
};

}// namespace triglav::graphics_api