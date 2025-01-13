#pragma once

#include "GraphicsApi.hpp"
#include "Pipeline.hpp"

#include <vector>

namespace triglav::graphics_api {

class Device;
class RenderTarget;
class Shader;

class PipelineBuilderBase
{
 public:
   explicit PipelineBuilderBase(Device& device);

   Index add_shader(const Shader& shader);
   void add_descriptor_binding(DescriptorType descriptorType, PipelineStageFlags shaderStages, u32 descriptorCount);
   void add_push_constant(PipelineStageFlags shaderStages, size_t size, size_t offset = 0);

 protected:
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
   explicit GraphicsPipelineBuilder(Device& device);

   GraphicsPipelineBuilder& fragment_shader(const Shader& shader);
   GraphicsPipelineBuilder& vertex_shader(const Shader& shader);
   GraphicsPipelineBuilder& vertex_attribute(const ColorFormat& format, size_t offset);
   GraphicsPipelineBuilder& end_vertex_layout();
   GraphicsPipelineBuilder& descriptor_binding(DescriptorType descriptorType, PipelineStage shaderStage);
   GraphicsPipelineBuilder& descriptor_binding_array(DescriptorType descriptorType, PipelineStage shaderStage, u32 descriptorCount);
   GraphicsPipelineBuilder& push_constant(PipelineStageFlags shaderStages, size_t size, size_t offset = 0);
   GraphicsPipelineBuilder& enable_depth_test(bool enabled);
   GraphicsPipelineBuilder& depth_test_mode(DepthTestMode mode);
   GraphicsPipelineBuilder& enable_blending(bool enabled);
   GraphicsPipelineBuilder& use_push_descriptors(bool enabled);
   GraphicsPipelineBuilder& vertex_topology(VertexTopology topology);
   GraphicsPipelineBuilder& rasterization_method(RasterizationMethod method);
   GraphicsPipelineBuilder& culling(Culling cull);
   GraphicsPipelineBuilder& begin_vertex_layout_raw(u32 strideSize);
   GraphicsPipelineBuilder& color_attachment(ColorFormat format);
   GraphicsPipelineBuilder& depth_attachment(ColorFormat format);

   [[nodiscard]] Result<Pipeline> build() const;

   template<typename TVertex>
   GraphicsPipelineBuilder& begin_vertex_layout()
   {
      return this->begin_vertex_layout_raw(sizeof(TVertex));
   }

 private:
   RenderTarget* m_renderTarget{};

   uint32_t m_vertexLocation{0};
   uint32_t m_vertexBinding{0};
   VkPolygonMode m_polygonMode = VK_POLYGON_MODE_FILL;
   VkPrimitiveTopology m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   VkFrontFace m_frontFace = VK_FRONT_FACE_CLOCKWISE;
   VkCullModeFlags m_cullMode = VK_CULL_MODE_FRONT_BIT;
   DepthTestMode m_depthTestMode{DepthTestMode::Disabled};
   bool m_blendingEnabled{true};
   std::vector<ColorFormat> m_colorAttachmentFormats;
   std::optional<ColorFormat> m_depthAttachmentFormat;

   std::vector<VkVertexInputBindingDescription> m_bindings{};
   std::vector<VkVertexInputAttributeDescription> m_attributes{};
};

}// namespace triglav::graphics_api