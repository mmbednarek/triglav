#include "PipelineBuilder.hpp"

#include "Device.hpp"
#include "RenderTarget.hpp"
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

   const auto index = m_shaderStageInfos.size();

   m_shaderStageInfos.emplace_back(info);

   return index;
}

void PipelineBuilderBase::add_descriptor_binding(const DescriptorType descriptorType, const PipelineStageFlags shaderStages,
                                                 const u32 descriptorCount)
{
   VkDescriptorSetLayoutBinding layoutBinding{};
   layoutBinding.descriptorCount = descriptorCount;
   layoutBinding.binding = m_vulkanDescriptorBindings.size();
   layoutBinding.descriptorType = vulkan::to_vulkan_descriptor_type(descriptorType);
   layoutBinding.stageFlags = vulkan::to_vulkan_shader_stage_flags(shaderStages);
   m_vulkanDescriptorBindings.emplace_back(layoutBinding);
}

void PipelineBuilderBase::add_push_constant(const PipelineStageFlags shaderStages, const size_t size, const size_t offset)
{
   VkPushConstantRange range;
   range.offset = offset;
   range.size = size;
   range.stageFlags = vulkan::to_vulkan_shader_stage_flags(shaderStages);
   m_pushConstantRanges.emplace_back(range);
}

Result<std::tuple<vulkan::DescriptorSetLayout, vulkan::PipelineLayout>> PipelineBuilderBase::build_pipeline_layout() const
{
   VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
   descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   descriptorSetLayoutInfo.bindingCount = m_vulkanDescriptorBindings.size();
   descriptorSetLayoutInfo.pBindings = m_vulkanDescriptorBindings.data();
   if (m_usePushDescriptors) {
      descriptorSetLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
   }

   vulkan::DescriptorSetLayout descriptorSetLayout(m_device.vulkan_device());
   if (descriptorSetLayout.construct(&descriptorSetLayoutInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
   pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount = 1;
   pipelineLayoutInfo.pSetLayouts = &(*descriptorSetLayout);
   pipelineLayoutInfo.pushConstantRangeCount = m_pushConstantRanges.size();
   pipelineLayoutInfo.pPushConstantRanges = m_pushConstantRanges.data();

   vulkan::PipelineLayout pipelineLayout(m_device.vulkan_device());
   if (const auto res = pipelineLayout.construct(&pipelineLayoutInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return std::make_tuple(std::move(descriptorSetLayout), std::move(pipelineLayout));
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

ComputePipelineBuilder& ComputePipelineBuilder::descriptor_binding(DescriptorType descriptorType)
{
   this->add_descriptor_binding(descriptorType, PipelineStage::ComputeShader, 1);
   return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::push_constant(PipelineStage shaderStage, size_t size, size_t offset)
{
   this->add_push_constant(shaderStage, size, offset);
   return *this;
}

Result<Pipeline> ComputePipelineBuilder::build() const
{
   if (m_shaderStageInfos.size() != 1) {
      return std::unexpected{Status::InvalidShaderStage};
   }

   auto layouts = this->build_pipeline_layout();
   if (not layouts.has_value()) {
      return std::unexpected(layouts.error());
   }
   auto&& [descriptorSetLayout, pipelineLayout] = *layouts;

   VkComputePipelineCreateInfo pipelineCreateInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
   pipelineCreateInfo.stage = m_shaderStageInfos.front();
   pipelineCreateInfo.layout = *pipelineLayout;

   vulkan::Pipeline pipeline(m_device.vulkan_device());

   VkPipeline vulkanPipeline;
   VkResult result = vkCreateComputePipelines(m_device.vulkan_device(), nullptr, 1, &pipelineCreateInfo, nullptr, &vulkanPipeline);
   if (result != VK_SUCCESS) {
      return std::unexpected{Status::PSOCreationFailed};
   }

   pipeline.take_ownership(vulkanPipeline);

   return Pipeline{std::move(pipelineLayout), std::move(pipeline), std::move(descriptorSetLayout), PipelineType::Compute};
}

ComputePipelineBuilder& ComputePipelineBuilder::use_push_descriptors(bool enabled)
{
   m_usePushDescriptors = enabled;
   return *this;
}

// -------------------------
// GRAPHICS PIPELINE BUILDER
// -------------------------

GraphicsPipelineBuilder::GraphicsPipelineBuilder(Device& device, RenderTarget& renderPass) :
    PipelineBuilderBase(device),
    m_renderTarget(&renderPass)
{
}

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
   VkVertexInputAttributeDescription vertexAttribute{};
   vertexAttribute.binding = m_vertexBinding;
   vertexAttribute.location = m_vertexLocation;
   vertexAttribute.format = *vulkan::to_vulkan_color_format(format);
   vertexAttribute.offset = offset;
   m_attributes.emplace_back(vertexAttribute);

   ++m_vertexLocation;

   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::end_vertex_layout()
{
   ++m_vertexBinding;
   m_vertexLocation = 0;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::descriptor_binding(const DescriptorType descriptorType, const PipelineStage shaderStage)
{
   this->add_descriptor_binding(descriptorType, shaderStage, 1);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::descriptor_binding_array(DescriptorType descriptorType, PipelineStage shaderStage,
                                                                           u32 descriptorCount)
{
   this->add_descriptor_binding(descriptorType, shaderStage, descriptorCount);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::push_constant(const PipelineStageFlags shaderStages, const size_t size,
                                                                const size_t offset)
{
   this->add_push_constant(shaderStages, size, offset);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enable_depth_test(const bool enabled)
{
   m_depthTestMode = enabled ? DepthTestMode::Enabled : DepthTestMode::Disabled;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::depth_test_mode(const DepthTestMode mode)
{
   m_depthTestMode = mode;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enable_blending(const bool enabled)
{
   m_blendingEnabled = enabled;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::use_push_descriptors(bool enabled)
{
   m_usePushDescriptors = enabled;
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::vertex_topology(const VertexTopology topology)
{
   switch (topology) {
   case VertexTopology::TriangleList:
      m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      break;
   case VertexTopology::TriangleFan:
      m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
      break;
   case VertexTopology::TriangleStrip:
      m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
      break;
   case VertexTopology::LineStrip:
      m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
      break;
   case VertexTopology::LineList:
      m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
      break;
   }
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::rasterization_method(const RasterizationMethod method)
{
   switch (method) {
   case RasterizationMethod::Point:
      m_polygonMode = VK_POLYGON_MODE_POINT;
      break;
   case RasterizationMethod::Line:
      m_polygonMode = VK_POLYGON_MODE_LINE;
      break;
   case RasterizationMethod::Fill:
      m_polygonMode = VK_POLYGON_MODE_FILL;
      break;
   }
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::culling(const Culling cull)
{
   switch (cull) {
   case Culling::Clockwise:
      m_cullMode = VK_CULL_MODE_FRONT_BIT;
      break;
   case Culling::CounterClockwise:
      m_cullMode = VK_CULL_MODE_BACK_BIT;
      break;
   case Culling::None:
      m_cullMode = VK_CULL_MODE_NONE;
      break;
   }
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::begin_vertex_layout_raw(const u32 strideSize)
{
   VkVertexInputBindingDescription binding{};
   binding.binding = m_vertexBinding;
   binding.stride = strideSize;
   binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
   m_bindings.emplace_back(binding);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::color_attachment(ColorFormat format)
{
   m_colorAttachmentFormats.emplace_back(format);
   return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::depth_attachment(ColorFormat format)
{
   m_depthAttachmentFormat.emplace(format);
   return *this;
}

constexpr std::array g_dynamicStates{
   VK_DYNAMIC_STATE_VIEWPORT,
   VK_DYNAMIC_STATE_SCISSOR,
};

Result<Pipeline> GraphicsPipelineBuilder::build() const
{
   VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
   dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicStateInfo.dynamicStateCount = g_dynamicStates.size();
   dynamicStateInfo.pDynamicStates = g_dynamicStates.data();

   VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
   vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputInfo.vertexBindingDescriptionCount = m_bindings.size();
   vertexInputInfo.pVertexBindingDescriptions = m_bindings.data();
   vertexInputInfo.vertexAttributeDescriptionCount = m_attributes.size();
   vertexInputInfo.pVertexAttributeDescriptions = m_attributes.data();

   VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
   inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssemblyInfo.topology = m_primitiveTopology;
   inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

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

   VkPipelineViewportStateCreateInfo viewportStateInfo{};
   viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportStateInfo.scissorCount = 1;
   viewportStateInfo.pScissors = &scissor;
   viewportStateInfo.viewportCount = 1;
   viewportStateInfo.pViewports = &viewport;

   VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{};
   rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizationStateInfo.depthClampEnable = VK_FALSE;
   rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
   rasterizationStateInfo.polygonMode = m_polygonMode;
   rasterizationStateInfo.lineWidth = 2.0f;
   rasterizationStateInfo.cullMode = m_cullMode;
   rasterizationStateInfo.frontFace = m_frontFace;
   rasterizationStateInfo.depthBiasEnable = VK_FALSE;
   rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
   rasterizationStateInfo.depthBiasClamp = 0.0f;
   rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;

   const auto sampleCount = m_renderTarget != nullptr ? m_renderTarget->sample_count() : SampleCount::Single;

   VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
   multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisamplingInfo.sampleShadingEnable = sampleCount != SampleCount::Single;
   multisamplingInfo.rasterizationSamples = static_cast<VkSampleCountFlagBits>(sampleCount);
   multisamplingInfo.minSampleShading = 1.0f;

   std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};
   if (m_renderTarget != nullptr) {
      for (int i = 0; i < m_renderTarget->color_attachment_count(); ++i) {
         VkPipelineColorBlendAttachmentState colorBlendAttachment{};
         colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
         colorBlendAttachment.blendEnable = m_blendingEnabled;
         colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
         colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
         colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
         colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
         colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
         colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MAX;
         colorBlendAttachments.emplace_back(colorBlendAttachment);
      }
   } else {
      for (const auto& _ : m_colorAttachmentFormats) {
         VkPipelineColorBlendAttachmentState colorBlendAttachment{};
         colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
         colorBlendAttachment.blendEnable = m_blendingEnabled;
         colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
         colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
         colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
         colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
         colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
         colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MAX;
         colorBlendAttachments.emplace_back(colorBlendAttachment);
      }
   }

   VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
   colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlendingInfo.logicOpEnable = not m_blendingEnabled;
   colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
   colorBlendingInfo.attachmentCount = colorBlendAttachments.size();
   colorBlendingInfo.pAttachments = colorBlendAttachments.data();
   colorBlendingInfo.blendConstants[0] = 0.0f;
   colorBlendingInfo.blendConstants[1] = 0.0f;
   colorBlendingInfo.blendConstants[2] = 0.0f;
   colorBlendingInfo.blendConstants[3] = 0.0f;

   VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
   depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depthStencilStateInfo.depthTestEnable = m_depthTestMode != DepthTestMode::Disabled;
   depthStencilStateInfo.depthWriteEnable = m_depthTestMode == DepthTestMode::Enabled;
   depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
   depthStencilStateInfo.depthBoundsTestEnable = false;
   depthStencilStateInfo.stencilTestEnable = false;

   auto layouts = this->build_pipeline_layout();
   if (not layouts.has_value()) {
      return std::unexpected(layouts.error());
   }
   auto&& [descriptorSetLayout, pipelineLayout] = *layouts;


   VkPipelineRenderingCreateInfo renderingInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

   std::vector<VkFormat> colorAttachmentFormats;
   for (const auto& colorFormat : m_colorAttachmentFormats) {
      colorAttachmentFormats.emplace_back(GAPI_CHECK(vulkan::to_vulkan_color_format(colorFormat)));
   }

   renderingInfo.colorAttachmentCount = colorAttachmentFormats.size();
   renderingInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
   if (m_depthAttachmentFormat.has_value()) {
      renderingInfo.depthAttachmentFormat = GAPI_CHECK(vulkan::to_vulkan_color_format(*m_depthAttachmentFormat));
   }

   VkGraphicsPipelineCreateInfo pipelineInfo{};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.layout = *pipelineLayout;
   pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStageInfos.size());
   pipelineInfo.pStages = m_shaderStageInfos.data();
   pipelineInfo.pMultisampleState = &multisamplingInfo;
   pipelineInfo.pDynamicState = &dynamicStateInfo;
   pipelineInfo.pRasterizationState = &rasterizationStateInfo;
   pipelineInfo.pViewportState = &viewportStateInfo;
   pipelineInfo.pColorBlendState = &colorBlendingInfo;
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
   pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
   pipelineInfo.renderPass = m_renderTarget != nullptr ? m_renderTarget->vulkan_render_pass() : nullptr;
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = nullptr;
   pipelineInfo.basePipelineIndex = -1;
   if (!m_colorAttachmentFormats.empty() || m_depthAttachmentFormat.has_value()) {
      pipelineInfo.pNext = &renderingInfo;
   }

   vulkan::Pipeline pipeline(m_device.vulkan_device());
   if (const auto res = pipeline.construct(nullptr, 1, &pipelineInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Pipeline{std::move(pipelineLayout), std::move(pipeline), std::move(descriptorSetLayout), PipelineType::Graphics};
}

}// namespace triglav::graphics_api
