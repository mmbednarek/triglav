#include "PipelineBuilder.h"

#include "Device.h"
#include "Shader.h"
#include "vulkan/Util.h"

namespace graphics_api {

PipelineBuilder::PipelineBuilder(Device &device, RenderPass &renderPass) :
    m_device(device),
    m_renderPass(renderPass)
{
}

PipelineBuilder &PipelineBuilder::fragment_shader(const Shader &shader)
{
   assert(shader.stage() == ShaderStage::Fragment);

   VkPipelineShaderStageCreateInfo info{};
   info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   info.module = *shader.vulkan_module();
   info.pName  = shader.name().data();
   info.stage  = vulkan::to_vulkan_shader_stage(shader.stage());
   m_shaderStageInfos.emplace_back(info);
   return *this;
}

PipelineBuilder &PipelineBuilder::vertex_shader(const Shader &shader)
{
   assert(shader.stage() == ShaderStage::Vertex);

   VkPipelineShaderStageCreateInfo info{};
   info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   info.module = *shader.vulkan_module();
   info.pName  = shader.name().data();
   info.stage  = vulkan::to_vulkan_shader_stage(shader.stage());
   m_shaderStageInfos.emplace_back(info);
   return *this;
}

PipelineBuilder &PipelineBuilder::vertex_attribute(const ColorFormat &format, const size_t offset)
{
   VkVertexInputAttributeDescription vertexAttribute{};
   vertexAttribute.binding  = m_vertexBinding;
   vertexAttribute.location = m_vertexLocation;
   vertexAttribute.format   = *vulkan::to_vulkan_color_format(format);
   vertexAttribute.offset   = offset;
   m_attributes.emplace_back(vertexAttribute);

   ++m_vertexLocation;

   return *this;
}

PipelineBuilder &PipelineBuilder::end_vertex_layout()
{
   ++m_vertexBinding;
   m_vertexLocation = 0;
   return *this;
}

PipelineBuilder &PipelineBuilder::descriptor_binding(const DescriptorType descriptorType,
                                                     const ShaderStage shaderStage)
{
   VkDescriptorSetLayoutBinding layoutBinding{};
   layoutBinding.descriptorCount = 1;
   layoutBinding.binding         = m_vulkanDescriptorBindings.size();
   layoutBinding.descriptorType  = vulkan::to_vulkan_descriptor_type(descriptorType);
   layoutBinding.stageFlags      = vulkan::to_vulkan_shader_stage_flags(ShaderStage::None | shaderStage);
   m_vulkanDescriptorBindings.emplace_back(layoutBinding);

   /*
   using graphics_api::DescriptorBinding;
   using graphics_api::DescriptorType;
   using graphics_api::ShaderStage;

   std::array<DescriptorBinding, 2> bindings{
      DescriptorBinding{0, DescriptorType::ImageSampler, ShaderStage::Fragment},
      DescriptorBinding{1, DescriptorType::ImageSampler, ShaderStage::Fragment},
      DescriptorBinding{2, DescriptorType::ImageSampler, ShaderStage::Fragment},
   };
   device.create_descriptor_set_layout(bindings);
   */
   return *this;
}

PipelineBuilder &PipelineBuilder::push_constant(const ShaderStage shaderStage, size_t size, size_t offset)
{
   VkPushConstantRange range;
   range.offset     = offset;
   range.size       = size;
   range.stageFlags = vulkan::to_vulkan_shader_stage_flags(ShaderStage::None | shaderStage);
   m_pushConstantRanges.emplace_back(range);

   return *this;
}

PipelineBuilder &PipelineBuilder::enable_depth_test(const bool enabled)
{
   m_depthTestEnabled = enabled;
   return *this;
}

PipelineBuilder &PipelineBuilder::vertex_topology(const VertexTopology topology)
{
   switch (topology) {
   case VertexTopology::TriangleList: m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
   case VertexTopology::TriangleFan: m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
   case VertexTopology::TriangleStrip: m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
   case VertexTopology::LineStrip: m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
   case VertexTopology::LineList: m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
   }
   return *this;
}

PipelineBuilder &PipelineBuilder::razterization_method(const RasterizationMethod method)
{
   switch (method) {
   case RasterizationMethod::Point: m_polygonMode = VK_POLYGON_MODE_POINT; break;
   case RasterizationMethod::Line: m_polygonMode = VK_POLYGON_MODE_LINE; break;
   case RasterizationMethod::Fill: m_polygonMode = VK_POLYGON_MODE_FILL; break;
   }
   return *this;
}

PipelineBuilder &PipelineBuilder::culling(const Culling cull)
{
   switch (cull) {
   case Culling::Clockwise: m_frontFace = VK_FRONT_FACE_CLOCKWISE; break;
   case Culling::CounterClockwise: m_frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
   }
   return *this;
}

constexpr std::array g_dynamicStates{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
};

Result<Pipeline> PipelineBuilder::build() const
{
   VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
   dynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicStateInfo.dynamicStateCount = g_dynamicStates.size();
   dynamicStateInfo.pDynamicStates    = g_dynamicStates.data();

   VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
   vertexInputInfo.sType                         = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputInfo.vertexBindingDescriptionCount = m_bindings.size();
   vertexInputInfo.pVertexBindingDescriptions    = m_bindings.data();
   vertexInputInfo.vertexAttributeDescriptionCount = m_attributes.size();
   vertexInputInfo.pVertexAttributeDescriptions    = m_attributes.data();

   VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
   inputAssemblyInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssemblyInfo.topology               = m_primitiveTopology;
   inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

   auto resolution = m_renderPass.resolution();

   VkViewport viewport{};
   viewport.x        = 0.0f;
   viewport.y        = 0.0f;
   viewport.width    = static_cast<float>(resolution.width);
   viewport.height   = static_cast<float>(resolution.height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 100.0f;

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = VkExtent2D{resolution.width, resolution.height};

   VkPipelineViewportStateCreateInfo viewportStateInfo{};
   viewportStateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportStateInfo.scissorCount  = 1;
   viewportStateInfo.pScissors     = &scissor;
   viewportStateInfo.viewportCount = 1;
   viewportStateInfo.pViewports    = &viewport;

   VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{};
   rasterizationStateInfo.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizationStateInfo.depthClampEnable = VK_FALSE;
   rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
   rasterizationStateInfo.polygonMode             = m_polygonMode;
   rasterizationStateInfo.lineWidth               = 2.0f;
   rasterizationStateInfo.cullMode                = VK_CULL_MODE_FRONT_BIT;
   rasterizationStateInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE;
   rasterizationStateInfo.depthBiasEnable         = VK_FALSE;
   rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
   rasterizationStateInfo.depthBiasClamp          = 0.0f;
   rasterizationStateInfo.depthBiasSlopeFactor    = 0.0f;

   const auto sampleCount = m_renderPass.sample_count();

   VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
   multisamplingInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisamplingInfo.sampleShadingEnable  = sampleCount != SampleCount::Single;
   multisamplingInfo.rasterizationSamples = static_cast<VkSampleCountFlagBits>(sampleCount);
   multisamplingInfo.minSampleShading     = 1.0f;

   std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};
   for (int i = 0; i < m_renderPass.color_attachment_count(); ++i) {
      VkPipelineColorBlendAttachmentState colorBlendAttachment{};
      colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      colorBlendAttachment.blendEnable         = VK_TRUE;
      colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
      colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
      colorBlendAttachments.emplace_back(colorBlendAttachment);
   }

   VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
   colorBlendingInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlendingInfo.logicOpEnable     = VK_TRUE;
   colorBlendingInfo.logicOp           = VK_LOGIC_OP_COPY;
   colorBlendingInfo.attachmentCount   = colorBlendAttachments.size();
   colorBlendingInfo.pAttachments      = colorBlendAttachments.data();
   colorBlendingInfo.blendConstants[0] = 0.0f;
   colorBlendingInfo.blendConstants[1] = 0.0f;
   colorBlendingInfo.blendConstants[2] = 0.0f;
   colorBlendingInfo.blendConstants[3] = 0.0f;

   VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
   descriptorSetLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   descriptorSetLayoutInfo.bindingCount = m_vulkanDescriptorBindings.size();
   descriptorSetLayoutInfo.pBindings    = m_vulkanDescriptorBindings.data();

   vulkan::DescriptorSetLayout descriptorSetLayout(m_device.vulkan_device());
   if (descriptorSetLayout.construct(&descriptorSetLayoutInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
   depthStencilStateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depthStencilStateInfo.depthTestEnable       = m_depthTestEnabled;
   depthStencilStateInfo.depthWriteEnable      = m_depthTestEnabled;
   depthStencilStateInfo.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
   depthStencilStateInfo.depthBoundsTestEnable = false;
   depthStencilStateInfo.stencilTestEnable     = false;

   VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
   pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount         = 1;
   pipelineLayoutInfo.pSetLayouts            = &(*descriptorSetLayout);
   pipelineLayoutInfo.pushConstantRangeCount = m_pushConstantRanges.size();
   pipelineLayoutInfo.pPushConstantRanges    = m_pushConstantRanges.data();

   vulkan::PipelineLayout pipelineLayout(m_device.vulkan_device());
   if (const auto res = pipelineLayout.construct(&pipelineLayoutInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   VkGraphicsPipelineCreateInfo pipelineInfo{};
   pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.layout              = *pipelineLayout;
   pipelineInfo.stageCount          = static_cast<uint32_t>(m_shaderStageInfos.size());
   pipelineInfo.pStages             = m_shaderStageInfos.data();
   pipelineInfo.pMultisampleState   = &multisamplingInfo;
   pipelineInfo.pDynamicState       = &dynamicStateInfo;
   pipelineInfo.pRasterizationState = &rasterizationStateInfo;
   pipelineInfo.pViewportState      = &viewportStateInfo;
   pipelineInfo.pColorBlendState    = &colorBlendingInfo;
   pipelineInfo.pVertexInputState   = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
   pipelineInfo.pDepthStencilState  = &depthStencilStateInfo;
   pipelineInfo.renderPass          = m_renderPass.vulkan_render_pass();
   pipelineInfo.subpass             = 0;
   pipelineInfo.basePipelineHandle  = nullptr;
   pipelineInfo.basePipelineIndex   = -1;

   vulkan::Pipeline pipeline(m_device.vulkan_device());
   if (const auto res = pipeline.construct(nullptr, 1, &pipelineInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Pipeline{std::move(pipelineLayout), std::move(pipeline), std::move(descriptorSetLayout)};
}

}// namespace graphics_api