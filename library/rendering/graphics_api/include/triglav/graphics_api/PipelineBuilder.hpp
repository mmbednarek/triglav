#pragma once

#include "GraphicsApi.hpp"
#include "Pipeline.hpp"

#include <vector>

namespace triglav::graphics_api {

class Device;
class Shader;

class PipelineBuilderBase
{
 public:
   explicit PipelineBuilderBase(Device& device);

   Index add_shader(const Shader& shader);
   void add_descriptor_binding(DescriptorType descriptor_type, PipelineStageFlags shader_stages, u32 descriptor_count);
   void add_push_constant(PipelineStageFlags shader_stages, size_t size, size_t offset = 0);

 protected:
   [[nodiscard]] Result<std::tuple<vulkan::DescriptorSetLayout, vulkan::PipelineLayout>> build_pipeline_layout() const;

   Device& m_device;
   std::vector<VkDescriptorSetLayoutBinding> m_vulkan_descriptor_bindings{};
   std::vector<VkPushConstantRange> m_push_constant_ranges{};
   std::vector<VkPipelineShaderStageCreateInfo> m_shader_stage_infos;
   bool m_use_push_descriptors{false};
};

class ComputePipelineBuilder : public PipelineBuilderBase
{
 public:
   explicit ComputePipelineBuilder(Device& device);

   ComputePipelineBuilder& compute_shader(const Shader& shader);
   ComputePipelineBuilder& descriptor_binding(DescriptorType descriptor_type);
   ComputePipelineBuilder& push_constant(PipelineStage shader_stage, size_t size, size_t offset = 0);
   ComputePipelineBuilder& use_push_descriptors(bool enabled);

   [[nodiscard]] Result<Pipeline> build() const;
};

class GraphicsPipelineBuilder : public PipelineBuilderBase
{
 public:
   explicit GraphicsPipelineBuilder(Device& device);

   GraphicsPipelineBuilder& fragment_shader(const Shader& shader);
   GraphicsPipelineBuilder& vertex_shader(const Shader& shader);
   GraphicsPipelineBuilder& vertex_attribute(const ColorFormat& format, size_t offset);
   GraphicsPipelineBuilder& end_vertex_layout();
   GraphicsPipelineBuilder& descriptor_binding(DescriptorType descriptor_type, PipelineStage shader_stage);
   GraphicsPipelineBuilder& descriptor_binding_array(DescriptorType descriptor_type, PipelineStage shader_stage, u32 descriptor_count);
   GraphicsPipelineBuilder& push_constant(PipelineStageFlags shader_stages, size_t size, size_t offset = 0);
   GraphicsPipelineBuilder& enable_depth_test(bool enabled);
   GraphicsPipelineBuilder& depth_test_mode(DepthTestMode mode);
   GraphicsPipelineBuilder& enable_blending(bool enabled);
   GraphicsPipelineBuilder& use_push_descriptors(bool enabled);
   GraphicsPipelineBuilder& vertex_topology(VertexTopology topology);
   GraphicsPipelineBuilder& rasterization_method(RasterizationMethod method);
   GraphicsPipelineBuilder& culling(Culling cull);
   GraphicsPipelineBuilder& begin_vertex_layout_raw(u32 stride_size);
   GraphicsPipelineBuilder& color_attachment(ColorFormat format);
   GraphicsPipelineBuilder& depth_attachment(ColorFormat format);
   GraphicsPipelineBuilder& line_width(float line_width);

   [[nodiscard]] Result<Pipeline> build() const;

   template<typename TVertex>
   GraphicsPipelineBuilder& begin_vertex_layout()
   {
      return this->begin_vertex_layout_raw(sizeof(TVertex));
   }

 private:
   uint32_t m_vertex_location{0};
   uint32_t m_vertex_binding{0};
   VkPolygonMode m_polygon_mode = VK_POLYGON_MODE_FILL;
   VkPrimitiveTopology m_primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   VkFrontFace m_front_face = VK_FRONT_FACE_CLOCKWISE;
   VkCullModeFlags m_cull_mode = VK_CULL_MODE_FRONT_BIT;
   DepthTestMode m_depth_test_mode{DepthTestMode::Disabled};
   bool m_blending_enabled{true};
   float m_line_width{1.0f};
   std::vector<ColorFormat> m_color_attachment_formats;
   std::optional<ColorFormat> m_depth_attachment_format;

   std::vector<VkVertexInputBindingDescription> m_bindings{};
   std::vector<VkVertexInputAttributeDescription> m_attributes{};
};

}// namespace triglav::graphics_api