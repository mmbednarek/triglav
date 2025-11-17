#include "PipelineBuilder.hpp"

#include "Device.hpp"
#include "Shader.hpp"
#include "vulkan/Util.hpp"

namespace triglav::graphics_api {

// -------------------------
// PIPELINE BUILDER BASE
// -------------------------

PipelineBuilderBase::PipelineBuilderBase(Device& device) :
    m_device(device)
{
}

Index PipelineBuilderBase::add_shader(const Shader& shader)
{
   VkPipelineShaderStageCreateInfo info{};
   info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   info.module = *shader.vulkan_module();
   info.pName = shader.name().data();
   info.stage = vulkan::to_vulkan_shader_stage(shader.stage());

   const auto index = m_shader_stage_infos.size();

   m_shader_stage_infos.emplace_back(info);

   return index;
}

void PipelineBuilderBase::add_descriptor_binding(const DescriptorType descriptor_type, const PipelineStageFlags shader_stages,
                                                 const u32 descriptor_count)
{
   VkDescriptorSetLayoutBinding layout_binding{};
   layout_binding.descriptorCount = descriptor_count;
   layout_binding.binding = static_cast<u32>(m_vulkan_descriptor_bindings.size());
   layout_binding.descriptorType = vulkan::to_vulkan_descriptor_type(descriptor_type);
   layout_binding.stageFlags = vulkan::to_vulkan_shader_stage_flags(shader_stages);
   m_vulkan_descriptor_bindings.emplace_back(layout_binding);
}

void PipelineBuilderBase::add_push_constant(const PipelineStageFlags shader_stages, const size_t size, const size_t offset)
{
   VkPushConstantRange range;
   range.offset = static_cast<u32>(offset);
   range.size = static_cast<u32>(size);
   range.stageFlags = vulkan::to_vulkan_shader_stage_flags(shader_stages);
   m_push_constant_ranges.emplace_back(range);
}

Result<std::tuple<vulkan::DescriptorSetLayout, vulkan::PipelineLayout>> PipelineBuilderBase::build_pipeline_layout() const
{
   VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
   descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   descriptor_set_layout_info.bindingCount = static_cast<u32>(m_vulkan_descriptor_bindings.size());
   descriptor_set_layout_info.pBindings = m_vulkan_descriptor_bindings.data();
   if (m_use_push_descriptors) {
      descriptor_set_layout_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
   }

   vulkan::DescriptorSetLayout descriptor_set_layout(m_device.vulkan_device());
   if (descriptor_set_layout.construct(&descriptor_set_layout_info) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   VkPipelineLayoutCreateInfo pipeline_layout_info{};
   pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipeline_layout_info.setLayoutCount = 1;
   pipeline_layout_info.pSetLayouts = &(*descriptor_set_layout);
   pipeline_layout_info.pushConstantRangeCount = static_cast<u32>(m_push_constant_ranges.size());
   pipeline_layout_info.pPushConstantRanges = m_push_constant_ranges.data();

   vulkan::PipelineLayout pipeline_layout(m_device.vulkan_device());
   if (const auto res = pipeline_layout.construct(&pipeline_layout_info); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return std::make_tuple(std::move(descriptor_set_layout), std::move(pipeline_layout));
}

// -------------------------
// COMPUTE PIPELINE BUILDER
// -------------------------

ComputePipelineBuilder::ComputePipelineBuilder(Device& device) :
    PipelineBuilderBase(device)
{
}

ComputePipelineBuilder& ComputePipelineBuilder::compute_shader(const Shader& shader)
{
   assert(shader.stage() == PipelineStage::ComputeShader);
   this->add_shader(shader);
   return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::descriptor_binding(DescriptorType descriptor_type)
{
   this->add_descriptor_binding(descriptor_type, PipelineStage::ComputeShader, 1);
   return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::push_constant(PipelineStage shader_stage, size_t size, size_t offset)
{
   this->add_push_constant(shader_stage, size, offset);
   return *this;
}

Result<Pipeline> ComputePipelineBuilder::build() const
{
   if (m_shader_stage_infos.size() != 1) {
      return std::unexpected{Status::InvalidShaderStage};
   }

   auto layouts = this->build_pipeline_layout();
   if (not layouts.has_value()) {
      return std::unexpected(layouts.error());
   }
   auto&& [descriptor_set_layout, pipeline_layout] = *layouts;

   VkComputePipelineCreateInfo pipeline_create_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
   pipeline_create_info.stage = m_shader_stage_infos.front();
   pipeline_create_info.layout = *pipeline_layout;

   vulkan::Pipeline pipeline(m_device.vulkan_device());

   VkPipeline vulkan_pipeline;
   VkResult result = vkCreateComputePipelines(m_device.vulkan_device(), nullptr, 1, &pipeline_create_info, nullptr, &vulkan_pipeline);
   if (result != VK_SUCCESS) {
      return std::unexpected{Status::PSOCreationFailed};
   }

   pipeline.take_ownership(vulkan_pipeline);

   return Pipeline{std::move(pipeline_layout), std::move(pipeline), std::move(descriptor_set_layout), PipelineType::Compute};
}

ComputePipelineBuilder& ComputePipelineBuilder::use_push_descriptors(bool enabled)
{
   m_use_push_descriptors = enabled;
   return *this;
}

// -------------------------
// GRAPHICS PIPELINE BUILDER
// -------------------------

GraphicsPipelineBuilder::GraphicsPipelineBuilder(Device& device) :
    PipelineBuilderBase(device)
{
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::fragment_shader(const Shader& shader)
{
   assert(shader.stage() == PipelineStage::FragmentShader);
   this->add_shader(shader);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::vertex_shader(const Shader& shader)
{
   assert(shader.stage() == PipelineStage::VertexShader);
   this->add_shader(shader);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::vertex_attribute(const ColorFormat& format, const size_t offset)
{
   VkVertexInputAttributeDescription vertex_attribute{};
   vertex_attribute.binding = m_vertex_binding;
   vertex_attribute.location = m_vertex_location;
   vertex_attribute.format = *vulkan::to_vulkan_color_format(format);
   vertex_attribute.offset = static_cast<u32>(offset);
   m_attributes.emplace_back(vertex_attribute);

   ++m_vertex_location;

   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::end_vertex_layout()
{
   ++m_vertex_binding;
   m_vertex_location = 0;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::descriptor_binding(const DescriptorType descriptor_type, const PipelineStage shader_stage)
{
   this->add_descriptor_binding(descriptor_type, shader_stage, 1);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::descriptor_binding_array(DescriptorType descriptor_type, PipelineStage shader_stage,
                                                                           u32 descriptor_count)
{
   this->add_descriptor_binding(descriptor_type, shader_stage, descriptor_count);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::push_constant(const PipelineStageFlags shader_stages, const size_t size,
                                                                const size_t offset)
{
   this->add_push_constant(shader_stages, size, offset);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enable_depth_test(const bool enabled)
{
   m_depth_test_mode = enabled ? DepthTestMode::Enabled : DepthTestMode::Disabled;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::depth_test_mode(const DepthTestMode mode)
{
   m_depth_test_mode = mode;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enable_blending(const bool enabled)
{
   m_blending_enabled = enabled;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::use_push_descriptors(bool enabled)
{
   m_use_push_descriptors = enabled;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::vertex_topology(const VertexTopology topology)
{
   switch (topology) {
   case VertexTopology::TriangleList:
      m_primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      break;
   case VertexTopology::TriangleFan:
      m_primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
      break;
   case VertexTopology::TriangleStrip:
      m_primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
      break;
   case VertexTopology::LineStrip:
      m_primitive_topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
      break;
   case VertexTopology::LineList:
      m_primitive_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
      break;
   }
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::rasterization_method(const RasterizationMethod method)
{
   switch (method) {
   case RasterizationMethod::Point:
      m_polygon_mode = VK_POLYGON_MODE_POINT;
      break;
   case RasterizationMethod::Line:
      m_polygon_mode = VK_POLYGON_MODE_LINE;
      break;
   case RasterizationMethod::Fill:
      m_polygon_mode = VK_POLYGON_MODE_FILL;
      break;
   }
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::culling(const Culling cull)
{
   switch (cull) {
   case Culling::Clockwise:
      m_cull_mode = VK_CULL_MODE_FRONT_BIT;
      break;
   case Culling::CounterClockwise:
      m_cull_mode = VK_CULL_MODE_BACK_BIT;
      break;
   case Culling::None:
      m_cull_mode = VK_CULL_MODE_NONE;
      break;
   }
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::begin_vertex_layout_raw(const u32 stride_size)
{
   VkVertexInputBindingDescription binding{};
   binding.binding = m_vertex_binding;
   binding.stride = stride_size;
   binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
   m_bindings.emplace_back(binding);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::color_attachment(ColorFormat format)
{
   m_color_attachment_formats.emplace_back(format);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::depth_attachment(ColorFormat format)
{
   m_depth_attachment_format.emplace(format);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::line_width(const float line_width)
{
   m_line_width = line_width;
   return *this;
}

constexpr std::array g_dynamic_states{
   VK_DYNAMIC_STATE_VIEWPORT,
   VK_DYNAMIC_STATE_SCISSOR,
};

Result<Pipeline> GraphicsPipelineBuilder::build() const
{
   VkPipelineDynamicStateCreateInfo dynamic_state_info{};
   dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamic_state_info.dynamicStateCount = static_cast<u32>(g_dynamic_states.size());
   dynamic_state_info.pDynamicStates = g_dynamic_states.data();

   VkPipelineVertexInputStateCreateInfo vertex_input_info{};
   vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertex_input_info.vertexBindingDescriptionCount = static_cast<u32>(m_bindings.size());
   vertex_input_info.pVertexBindingDescriptions = m_bindings.data();
   vertex_input_info.vertexAttributeDescriptionCount = static_cast<u32>(m_attributes.size());
   vertex_input_info.pVertexAttributeDescriptions = m_attributes.data();

   VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
   input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   input_assembly_info.topology = m_primitive_topology;
   input_assembly_info.primitiveRestartEnable = VK_FALSE;

   Resolution resolution{800, 800};

   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(resolution.width);
   viewport.height = static_cast<float>(resolution.height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 100.0f;

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = VkExtent2D{resolution.width, resolution.height};

   VkPipelineViewportStateCreateInfo viewport_state_info{};
   viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewport_state_info.scissorCount = 1;
   viewport_state_info.pScissors = &scissor;
   viewport_state_info.viewportCount = 1;
   viewport_state_info.pViewports = &viewport;

   VkPipelineRasterizationStateCreateInfo rasterization_state_info{};
   rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterization_state_info.depthClampEnable = VK_FALSE;
   rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
   rasterization_state_info.polygonMode = m_polygon_mode;
   rasterization_state_info.lineWidth = m_line_width;
   rasterization_state_info.cullMode = m_cull_mode;
   rasterization_state_info.frontFace = m_front_face;
   rasterization_state_info.depthBiasEnable = VK_FALSE;
   rasterization_state_info.depthBiasConstantFactor = 0.0f;
   rasterization_state_info.depthBiasClamp = 0.0f;
   rasterization_state_info.depthBiasSlopeFactor = 0.0f;

   VkPipelineMultisampleStateCreateInfo multisampling_info{};
   multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling_info.sampleShadingEnable = false;
   multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   multisampling_info.minSampleShading = 1.0f;

   std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments{};
   for ([[maybe_unused]] const auto& _ : m_color_attachment_formats) {
      VkPipelineColorBlendAttachmentState color_blend_attachment{};
      color_blend_attachment.colorWriteMask =
         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      color_blend_attachment.blendEnable = m_blending_enabled;
      color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
      color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
      color_blend_attachment.alphaBlendOp = VK_BLEND_OP_MAX;
      color_blend_attachments.emplace_back(color_blend_attachment);
   }

   VkPipelineColorBlendStateCreateInfo color_blending_info{};
   color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   color_blending_info.logicOpEnable = not m_blending_enabled;
   color_blending_info.logicOp = VK_LOGIC_OP_COPY;
   color_blending_info.attachmentCount = static_cast<u32>(color_blend_attachments.size());
   color_blending_info.pAttachments = color_blend_attachments.data();
   color_blending_info.blendConstants[0] = 0.0f;
   color_blending_info.blendConstants[1] = 0.0f;
   color_blending_info.blendConstants[2] = 0.0f;
   color_blending_info.blendConstants[3] = 0.0f;

   VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
   depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depth_stencil_state_info.depthTestEnable = m_depth_test_mode != DepthTestMode::Disabled;
   depth_stencil_state_info.depthWriteEnable = m_depth_test_mode == DepthTestMode::Enabled;
   depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
   depth_stencil_state_info.depthBoundsTestEnable = false;
   depth_stencil_state_info.stencilTestEnable = false;

   auto layouts = this->build_pipeline_layout();
   if (not layouts.has_value()) {
      return std::unexpected(layouts.error());
   }
   auto&& [descriptor_set_layout, pipeline_layout] = *layouts;


   VkPipelineRenderingCreateInfo rendering_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

   std::vector<VkFormat> color_attachment_formats;
   for (const auto& color_format : m_color_attachment_formats) {
      color_attachment_formats.emplace_back(GAPI_CHECK(vulkan::to_vulkan_color_format(color_format)));
   }

   rendering_info.colorAttachmentCount = static_cast<u32>(color_attachment_formats.size());
   rendering_info.pColorAttachmentFormats = color_attachment_formats.data();
   if (m_depth_attachment_format.has_value()) {
      rendering_info.depthAttachmentFormat = GAPI_CHECK(vulkan::to_vulkan_color_format(*m_depth_attachment_format));
   }

   VkGraphicsPipelineCreateInfo pipeline_info{};
   pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipeline_info.layout = *pipeline_layout;
   pipeline_info.stageCount = static_cast<uint32_t>(m_shader_stage_infos.size());
   pipeline_info.pStages = m_shader_stage_infos.data();
   pipeline_info.pMultisampleState = &multisampling_info;
   pipeline_info.pDynamicState = &dynamic_state_info;
   pipeline_info.pRasterizationState = &rasterization_state_info;
   pipeline_info.pViewportState = &viewport_state_info;
   pipeline_info.pColorBlendState = &color_blending_info;
   pipeline_info.pVertexInputState = &vertex_input_info;
   pipeline_info.pInputAssemblyState = &input_assembly_info;
   pipeline_info.pDepthStencilState = &depth_stencil_state_info;
   pipeline_info.renderPass = nullptr;
   pipeline_info.subpass = 0;
   pipeline_info.basePipelineHandle = nullptr;
   pipeline_info.basePipelineIndex = -1;
   if (!m_color_attachment_formats.empty() || m_depth_attachment_format.has_value()) {
      pipeline_info.pNext = &rendering_info;
   }

   vulkan::Pipeline pipeline(m_device.vulkan_device());
   if (const auto res = pipeline.construct(nullptr, 1, &pipeline_info); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Pipeline{std::move(pipeline_layout), std::move(pipeline), std::move(descriptor_set_layout), PipelineType::Graphics};
}

}// namespace triglav::graphics_api
