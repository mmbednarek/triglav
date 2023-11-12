#include "Device.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <format>
#include <iostream>
#include <ostream>
#include <set>
#include <shared_mutex>
#include <vector>

#include "vulkan/Util.h"

#undef max

using graphics_api::BufferPurpose;

namespace {
bool physical_device_pick_predicate(VkPhysicalDevice physicalDevice)
{
   VkPhysicalDeviceProperties props{};
   vkGetPhysicalDeviceProperties(physicalDevice, &props);
   return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

bool graphics_family_predicate(const VkQueueFamilyProperties &properties)
{
   return (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
}

bool present_only_family_predicate(const VkQueueFamilyProperties &properties)
{
   return ((properties.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) &&
          ((properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0);
}

bool present_family_predicate(const VkQueueFamilyProperties &properties)
{
   return (properties.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
}

VkBufferUsageFlags map_buffer_purpose_to_usage_flags(const BufferPurpose purpose)
{
   switch (purpose) {
   case BufferPurpose::TransferBuffer: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
   case BufferPurpose::VertexBuffer:
      return VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
   case BufferPurpose::UniformBuffer: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
   }
   return 0;
}

VkMemoryPropertyFlags map_buffer_purpose_to_memory_properties(const BufferPurpose purpose)
{
   switch (purpose) {
   case BufferPurpose::TransferBuffer://fallthrough
   case BufferPurpose::UniformBuffer:
      return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   case BufferPurpose::VertexBuffer: return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   }
   return 0;
}

VKAPI_ATTR VkBool32 VKAPI_CALL validation_layers_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
   std::cerr << "[VALIDATION] " << pCallbackData->pMessage << '\n';
   return VK_FALSE;
}
}// namespace

namespace graphics_api {
DECLARE_VLK_ENUMERATOR(get_physical_devices, VkPhysicalDevice, vkEnumeratePhysicalDevices)
DECLARE_VLK_ENUMERATOR(get_queue_family_properties, VkQueueFamilyProperties,
                       vkGetPhysicalDeviceQueueFamilyProperties)
DECLARE_VLK_ENUMERATOR(get_surface_formats, VkSurfaceFormatKHR, vkGetPhysicalDeviceSurfaceFormatsKHR)
DECLARE_VLK_ENUMERATOR(get_swapchain_images, VkImage, vkGetSwapchainImagesKHR)

Device::Device(vulkan::Instance instance,
#if GAPI_ENABLE_VALIDATION
               vulkan::DebugUtilsMessengerEXT debugMessenger,
#endif
               vulkan::SurfaceKHR surface, vulkan::Device device, VkPhysicalDevice physicalDevice,
               const vulkan::QueueFamilyIndices &queueFamilies, vulkan::CommandPool commandPool) :
    m_instance(std::move(instance)),
#if GAPI_ENABLE_VALIDATION
    m_debugMessenger(std::move(debugMessenger)),
#endif
    m_surface(std::move(surface)),
    m_device(std::move(device)),
    m_physicalDevice(physicalDevice),
    m_swapchain(*m_device),
    m_queueFamilies(queueFamilies),
    m_renderPass(*m_device),
    m_commandPool(std::move(commandPool)),
    m_sampler(*m_device)
{
   vkGetDeviceQueue(*m_device, m_queueFamilies.graphicsQueue, 0, &m_graphicsQueue);
   vkGetDeviceQueue(*m_device, m_queueFamilies.presentQueue, 0, &m_presentQueue);
}

bool Device::is_surface_format_supported(const ColorFormat &colorFormat, const ColorSpace colorSpace) const
{
   const auto formats = vulkan::get_surface_formats(m_physicalDevice, *m_surface);

   const auto targetVulkanFormat = vulkan::to_vulkan_color_format(colorFormat);
   if (not targetVulkanFormat.has_value()) {
      return false;
   }

   const auto targetVulkanColorSpace = vulkan::to_vulkan_color_space(colorSpace);
   if (not targetVulkanColorSpace.has_value()) {
      return false;
   }

   for (const auto [vulkanFormat, vulkanColorSpace] : formats) {
      if (vulkanFormat != targetVulkanFormat)
         continue;
      if (vulkanColorSpace != targetVulkanColorSpace)
         continue;

      return true;
   }

   return false;
}

std::pair<Resolution, Resolution> Device::get_surface_resolution_limits() const
{
   VkSurfaceCapabilitiesKHR capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, *m_surface, &capabilities);

   return {
           Resolution{capabilities.minImageExtent.width, capabilities.minImageExtent.height},
           Resolution{capabilities.maxImageExtent.width, capabilities.maxImageExtent.height}
   };
}

Result<Shader> Device::create_shader(const ShaderStage stage, const std::string_view entrypoint,
                                     const std::span<const uint8_t> code)
{
   VkShaderModuleCreateInfo shaderModuleInfo{};
   shaderModuleInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   shaderModuleInfo.codeSize = code.size();
   shaderModuleInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

   vulkan::ShaderModule shaderModule(*m_device);
   if (shaderModule.construct(&shaderModuleInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Shader(std::string{entrypoint}, stage, std::move(shaderModule));
}

Result<CommandList> Device::create_command_list() const
{
   VkCommandBufferAllocateInfo allocateInfo{};
   allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocateInfo.commandPool        = *m_commandPool;
   allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocateInfo.commandBufferCount = 1;

   VkCommandBuffer commandBuffer;
   if (vkAllocateCommandBuffers(*m_device, &allocateInfo, &commandBuffer) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return CommandList(commandBuffer, *m_device, *m_commandPool);
}

Status Device::init_swapchain(const Resolution &resolution)
{
   const auto vulkanColorFormat = vulkan::to_vulkan_color_format(m_surfaceFormat);
   if (not vulkanColorFormat.has_value())
      return Status::UnsupportedFormat;

   const auto vulkanColorSpace = vulkan::to_vulkan_color_space(m_colorSpace);
   if (not vulkanColorSpace.has_value())
      return Status::UnsupportedFormat;

   VkSurfaceCapabilitiesKHR capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, *m_surface, &capabilities);

   VkSwapchainCreateInfoKHR swapchainInfo{};
   swapchainInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   swapchainInfo.surface          = *m_surface;
   swapchainInfo.presentMode      = VK_PRESENT_MODE_MAILBOX_KHR;
   swapchainInfo.imageExtent      = VkExtent2D{resolution.width, resolution.height};
   swapchainInfo.imageFormat      = *vulkanColorFormat;
   swapchainInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   swapchainInfo.oldSwapchain     = *m_swapchain;
   swapchainInfo.minImageCount    = capabilities.maxImageCount;
   swapchainInfo.imageColorSpace  = *vulkanColorSpace;
   swapchainInfo.imageArrayLayers = 1;
   swapchainInfo.preTransform     = capabilities.currentTransform;
   swapchainInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   swapchainInfo.clipped          = true;

   const std::array<uint32_t, 2> queueFamilyIndices{m_queueFamilies.graphicsQueue,
                                                    m_queueFamilies.presentQueue};
   if (m_queueFamilies.graphicsQueue != m_queueFamilies.presentQueue) {
      swapchainInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      swapchainInfo.queueFamilyIndexCount = queueFamilyIndices.size();
      swapchainInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
   } else {
      swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   }

   if (m_swapchain.construct(&swapchainInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   m_surfaceResolution = resolution;
   m_swapchainImageViews.clear();

   const auto images = vulkan::get_swapchain_images(*m_device, *m_swapchain);
   for (const auto image : images) {
      VkImageViewCreateInfo imageViewInfo{};
      imageViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageViewInfo.image                           = image;
      imageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      imageViewInfo.format                          = *vulkanColorFormat;
      imageViewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imageViewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imageViewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imageViewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      imageViewInfo.subresourceRange.baseMipLevel   = 0;
      imageViewInfo.subresourceRange.levelCount     = 1;
      imageViewInfo.subresourceRange.baseArrayLayer = 0;
      imageViewInfo.subresourceRange.layerCount     = 1;

      vulkan::ImageView imageView(*m_device);
      if (imageView.construct(&imageViewInfo) != VK_SUCCESS) {
         return Status::UnsupportedDevice;
      }

      m_swapchainImageViews.emplace_back(std::move(imageView));
   }

   m_swapchainFramebuffers.clear();

   for (const auto &imageView : m_swapchainImageViews) {
      std::array<VkImageView, 1> attachments{*imageView};

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass      = *m_renderPass;
      framebufferInfo.attachmentCount = attachments.size();
      framebufferInfo.pAttachments    = attachments.data();
      framebufferInfo.width           = m_surfaceResolution.width;
      framebufferInfo.height          = m_surfaceResolution.height;
      framebufferInfo.layers          = 1;

      vulkan::Framebuffer framebuffer(*m_device);
      if (framebuffer.construct(&framebufferInfo) != VK_SUCCESS) {
         return Status::UnsupportedDevice;
      }

      m_swapchainFramebuffers.emplace_back(std::move(framebuffer));
   }

   return Status::Success;
}

constexpr std::array<VkDynamicState, 2> g_dynamicStates{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
};

Result<Pipeline> Device::create_pipeline(const std::span<Shader *> shaders,
                                         const std::span<VertexInputLayout> layouts,
                                         std::span<DescriptorBinding> descriptorBindings)
{
   std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
   shaderStageInfos.resize(shaders.size());

   std::transform(shaders.begin(), shaders.end(), shaderStageInfos.begin(), [](const Shader *shader) {
      VkPipelineShaderStageCreateInfo info{};
      info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      info.module = *shader->vulkan_module();
      info.pName  = shader->name().data();
      info.stage  = vulkan::to_vulkan_shader_stage(shader->stage());
      return info;
   });

   VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
   dynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicStateInfo.dynamicStateCount = g_dynamicStates.size();
   dynamicStateInfo.pDynamicStates    = g_dynamicStates.data();

   size_t totalAttributeCount = 0;

   std::vector<VkVertexInputBindingDescription> bindings{};
   bindings.resize(layouts.size());
   std::transform(layouts.begin(), layouts.end(), bindings.begin(),
                  [i = 0, &totalAttributeCount](const VertexInputLayout &layout) mutable {
                     VkVertexInputBindingDescription binding{};
                     binding.binding   = i;
                     binding.stride    = layout.structure_size;
                     binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                     totalAttributeCount += layout.attributes.size();
                     ++i;
                     return binding;
                  });

   std::vector<VkVertexInputAttributeDescription> attributes{};
   attributes.resize(totalAttributeCount);

   size_t binding_id = 0;
   size_t i          = 0;
   for (const auto &layout : layouts) {
      for (const auto &attribute : layout.attributes) {
         attributes[i].binding  = binding_id;
         attributes[i].location = attribute.location;
         attributes[i].format   = *vulkan::to_vulkan_color_format(attribute.format);
         attributes[i].offset   = attribute.offset;
         ++i;
      }
      ++binding_id;
   }

   VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
   vertexInputInfo.sType                         = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputInfo.vertexBindingDescriptionCount = bindings.size();
   vertexInputInfo.pVertexBindingDescriptions    = bindings.data();
   vertexInputInfo.vertexAttributeDescriptionCount = attributes.size();
   vertexInputInfo.pVertexAttributeDescriptions    = attributes.data();

   VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
   inputAssemblyInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssemblyInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

   VkViewport viewport{};
   viewport.x        = 0.0f;
   viewport.y        = 0.0f;
   viewport.width    = static_cast<float>(m_surfaceResolution.width);
   viewport.height   = static_cast<float>(m_surfaceResolution.height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 100.0f;

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = VkExtent2D{m_surfaceResolution.width, m_surfaceResolution.height};

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
   rasterizationStateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
   rasterizationStateInfo.lineWidth               = 1.0f;
   rasterizationStateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
   rasterizationStateInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE;
   rasterizationStateInfo.depthBiasEnable         = VK_FALSE;
   rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
   rasterizationStateInfo.depthBiasClamp          = 0.0f;
   rasterizationStateInfo.depthBiasSlopeFactor    = 0.0f;

   VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
   multisamplingInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisamplingInfo.sampleShadingEnable  = VK_FALSE;
   multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   multisamplingInfo.minSampleShading     = 1.0f;

   VkPipelineColorBlendAttachmentState colorBlendAttachment{};
   colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable         = VK_FALSE;
   colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
   colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
   colorBlendingInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlendingInfo.logicOpEnable     = VK_FALSE;
   colorBlendingInfo.logicOp           = VK_LOGIC_OP_COPY;
   colorBlendingInfo.attachmentCount   = 1;
   colorBlendingInfo.pAttachments      = &colorBlendAttachment;
   colorBlendingInfo.blendConstants[0] = 0.0f;
   colorBlendingInfo.blendConstants[1] = 0.0f;
   colorBlendingInfo.blendConstants[2] = 0.0f;
   colorBlendingInfo.blendConstants[3] = 0.0f;

   std::vector<VkDescriptorSetLayoutBinding> vulkanDescriptorBindings{};
   vulkanDescriptorBindings.resize(descriptorBindings.size());
   std::transform(descriptorBindings.begin(), descriptorBindings.end(), vulkanDescriptorBindings.begin(),
                  [](const DescriptorBinding &binding) {
                     VkDescriptorSetLayoutBinding result{};
                     result.descriptorCount = binding.descriptorCount;
                     result.binding         = binding.binding;
                     result.descriptorType  = vulkan::to_vulkan_descriptor_type(binding.type);
                     result.stageFlags      = vulkan::to_vulkan_shader_stage_flags(binding.shaderStages);
                     return result;
                  });

   VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
   descriptorSetLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   descriptorSetLayoutInfo.bindingCount = vulkanDescriptorBindings.size();
   descriptorSetLayoutInfo.pBindings    = vulkanDescriptorBindings.data();

   vulkan::DescriptorSetLayout descriptorSetLayout(*m_device);
   if (descriptorSetLayout.construct(&descriptorSetLayoutInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
   pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount = 1;
   pipelineLayoutInfo.pSetLayouts    = &(*descriptorSetLayout);

   vulkan::PipelineLayout pipelineLayout(*m_device);
   if (const auto res = pipelineLayout.construct(&pipelineLayoutInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   VkGraphicsPipelineCreateInfo pipelineInfo{};
   pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.layout              = *pipelineLayout;
   pipelineInfo.stageCount          = static_cast<uint32_t>(shaderStageInfos.size());
   pipelineInfo.pStages             = shaderStageInfos.data();
   pipelineInfo.pMultisampleState   = &multisamplingInfo;
   pipelineInfo.pDynamicState       = &dynamicStateInfo;
   pipelineInfo.pRasterizationState = &rasterizationStateInfo;
   pipelineInfo.pViewportState      = &viewportStateInfo;
   pipelineInfo.pColorBlendState    = &colorBlendingInfo;
   pipelineInfo.pVertexInputState   = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
   pipelineInfo.renderPass          = *m_renderPass;
   pipelineInfo.subpass             = 0;
   pipelineInfo.basePipelineHandle  = nullptr;
   pipelineInfo.basePipelineIndex   = -1;

   vulkan::Pipeline pipeline(*m_device);
   if (const auto res = pipeline.construct(nullptr, 1, &pipelineInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes{
           VkDescriptorPoolSize{
                   .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                   .descriptorCount = static_cast<uint32_t>(m_swapchainImageViews.size()),
           },
           {
                   .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                   .descriptorCount = static_cast<uint32_t>(m_swapchainImageViews.size()),
           },
   };

   VkDescriptorPoolCreateInfo descriptorPoolInfo{};
   descriptorPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   descriptorPoolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
   descriptorPoolInfo.poolSizeCount = descriptorPoolSizes.size();
   descriptorPoolInfo.pPoolSizes    = descriptorPoolSizes.data();
   descriptorPoolInfo.maxSets       = m_swapchainImageViews.size();

   vulkan::DescriptorPool descriptorPool{*m_device};
   if (descriptorPool.construct(&descriptorPoolInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   std::vector<VkDescriptorSetLayout> descriptorLayouts{};
   descriptorLayouts.resize(m_swapchainImageViews.size(), *descriptorSetLayout);

   VkDescriptorSetAllocateInfo descriptorSetsInfo{};
   descriptorSetsInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   descriptorSetsInfo.descriptorPool     = *descriptorPool;
   descriptorSetsInfo.descriptorSetCount = descriptorLayouts.size();
   descriptorSetsInfo.pSetLayouts        = descriptorLayouts.data();

   std::vector<VkDescriptorSet> descriptorSets{};
   descriptorSets.resize(m_swapchainImageViews.size());
   if (auto res = vkAllocateDescriptorSets(*m_device, &descriptorSetsInfo, descriptorSets.data()); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Pipeline{std::move(pipelineLayout), std::move(pipeline), std::move(descriptorPool),
                   std::move(descriptorSetLayout), std::move(descriptorSets), *m_sampler};
}

Result<Buffer> Device::create_buffer(const BufferPurpose purpose, const uint64_t size)
{
   VkBufferCreateInfo bufferInfo{};
   bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size        = size;
   bufferInfo.usage       = map_buffer_purpose_to_usage_flags(purpose);
   bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

   vulkan::Buffer buffer(*m_device);
   if (const auto res = buffer.construct(&bufferInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   VkMemoryRequirements memRequirements;
   vkGetBufferMemoryRequirements(*m_device, *buffer, &memRequirements);

   VkMemoryAllocateInfo allocateInfo{};
   allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocateInfo.allocationSize  = size;
   allocateInfo.memoryTypeIndex = this->find_memory_type(memRequirements.memoryTypeBits,
                                                         map_buffer_purpose_to_memory_properties(purpose));

   vulkan::DeviceMemory memory(*m_device);
   if (memory.construct(&allocateInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   if (vkBindBufferMemory(*m_device, *buffer, *memory, 0) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Buffer(size, std::move(buffer), std::move(memory));
}

Result<Fence> Device::create_fence() const
{
   VkFenceCreateInfo fenceInfo{};
   fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

   vulkan::Fence fence(*m_device);
   if (fence.construct(&fenceInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Fence(std::move(fence));
}

Result<Semaphore> Device::create_semaphore() const
{
   VkSemaphoreCreateInfo semaphoreInfo{};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   vulkan::Semaphore semaphore(*m_device);
   if (semaphore.construct(&semaphoreInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Semaphore(std::move(semaphore));
}

Status Device::begin_graphic_commands(CommandList &commandList, uint32_t framebufferIndex,
                                      const Color &clearColor) const
{
   assert(framebufferIndex < m_swapchainFramebuffers.size());

   commandList.set_is_one_time(false);

   vkResetCommandBuffer(commandList.vulkan_command_buffer(), 0);

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   if (vkBeginCommandBuffer(commandList.vulkan_command_buffer(), &beginInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   VkClearValue clearValue;
   clearValue.color = {
           {clearColor.r, clearColor.g, clearColor.b, clearColor.a}
   };

   VkRenderPassBeginInfo renderPassInfo{};
   renderPassInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass               = *m_renderPass;
   renderPassInfo.framebuffer              = *m_swapchainFramebuffers[framebufferIndex];
   renderPassInfo.renderArea.offset        = {0, 0};
   renderPassInfo.renderArea.extent.width  = m_surfaceResolution.width;
   renderPassInfo.renderArea.extent.height = m_surfaceResolution.height;
   renderPassInfo.clearValueCount          = 1;
   renderPassInfo.pClearValues             = &clearValue;

   vkCmdBeginRenderPass(commandList.vulkan_command_buffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

   VkViewport viewport{};
   viewport.x        = 0.0f;
   viewport.y        = 0.0f;
   viewport.width    = static_cast<float>(m_surfaceResolution.width);
   viewport.height   = static_cast<float>(m_surfaceResolution.height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   vkCmdSetViewport(commandList.vulkan_command_buffer(), 0, 1, &viewport);

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = VkExtent2D{m_surfaceResolution.width, m_surfaceResolution.height};
   vkCmdSetScissor(commandList.vulkan_command_buffer(), 0, 1, &scissor);

   return Status::Success;
}

Status Device::begin_one_time_commands(CommandList &commandList) const
{
   commandList.set_is_one_time(true);

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   if (vkBeginCommandBuffer(commandList.vulkan_command_buffer(), &beginInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
}

uint32_t Device::get_available_framebuffer(const Semaphore &semaphore) const
{
   uint32_t imageIndex;
   vkAcquireNextImageKHR(*m_device, *m_swapchain, UINT64_MAX, semaphore.vulkan_semaphore(), VK_NULL_HANDLE,
                         &imageIndex);
   return imageIndex;
}

Status Device::submit_command_list(const CommandList &commandList, const Semaphore &waitSemaphore,
                                   const Semaphore &signalSemaphore, const Fence &fence) const
{
   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   const std::array waitSemaphores{waitSemaphore.vulkan_semaphore()};
   const std::array signalSemaphores{signalSemaphore.vulkan_semaphore()};
   static constexpr std::array<VkPipelineStageFlags, 1> waitStages{
           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
   const std::array commandBuffers{commandList.vulkan_command_buffer()};

   submitInfo.waitSemaphoreCount   = waitSemaphores.size();
   submitInfo.pWaitSemaphores      = waitSemaphores.data();
   submitInfo.pWaitDstStageMask    = waitStages.data();
   submitInfo.commandBufferCount   = commandBuffers.size();
   submitInfo.pCommandBuffers      = commandBuffers.data();
   submitInfo.signalSemaphoreCount = signalSemaphores.size();
   submitInfo.pSignalSemaphores    = signalSemaphores.data();

   if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, fence.vulkan_fence()) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status Device::present(const Semaphore &semaphore, const uint32_t framebufferIndex) const
{
   std::array<VkSemaphore, 1> waitSemaphores{semaphore.vulkan_semaphore()};
   std::array<VkSwapchainKHR, 1> swapchains{*m_swapchain};
   std::array<uint32_t, 1> imageIndices{framebufferIndex};

   VkPresentInfoKHR presentInfo{};
   presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   presentInfo.waitSemaphoreCount = waitSemaphores.size();
   presentInfo.pWaitSemaphores    = waitSemaphores.data();
   presentInfo.swapchainCount     = swapchains.size();
   presentInfo.pSwapchains        = swapchains.data();
   presentInfo.pImageIndices      = imageIndices.data();
   presentInfo.pResults           = nullptr;

   if (vkQueuePresentKHR(m_presentQueue, &presentInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

void Device::await_all() const
{
   vkDeviceWaitIdle(*m_device);
}

uint32_t Device::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
   VkPhysicalDeviceMemoryProperties memProperties;
   vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

   for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
         return i;
      }
   }

   return 0;
}

Status Device::init_color_format(const ColorFormat &colorFormat, ColorSpace colorSpace)
{
   if (not this->is_surface_format_supported(colorFormat, colorSpace))
      return Status::UnsupportedFormat;

   m_surfaceFormat = colorFormat;
   m_colorSpace    = colorSpace;

   VkAttachmentDescription colorAttachment{};
   colorAttachment.format  = vulkan::to_vulkan_color_format(m_surfaceFormat).value_or(VK_FORMAT_UNDEFINED);
   colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
   colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentReference colorAttachmentRef{};
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass{};
   subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments    = &colorAttachmentRef;

   VkRenderPassCreateInfo renderPassInfo{};
   renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = 1;
   renderPassInfo.pAttachments    = &colorAttachment;
   renderPassInfo.subpassCount    = 1;
   renderPassInfo.pSubpasses      = &subpass;

   if (const auto res = m_renderPass.construct(&renderPassInfo); res != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   VkPhysicalDeviceProperties properties{};
   vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

   VkSamplerCreateInfo samplerInfo{};
   samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   samplerInfo.magFilter = VK_FILTER_LINEAR;
   samplerInfo.minFilter = VK_FILTER_LINEAR;
   samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.anisotropyEnable = VK_FALSE;
   samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
   samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   samplerInfo.unnormalizedCoordinates = VK_FALSE;
   samplerInfo.compareEnable = VK_FALSE;
   samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
   samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

   if (m_sampler.construct(&samplerInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status Device::submit_command_list_one_time(const CommandList &commandList) const
{
   const auto vulkanCommandList = commandList.vulkan_command_buffer();

   VkSubmitInfo submitInfo{};
   submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers    = &vulkanCommandList;

   vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
   vkQueueWaitIdle(m_graphicsQueue);

   return Status::Success;
}

uint32_t Device::framebuffer_count() const
{
   return m_swapchainImageViews.size();
}

Result<Texture> Device::create_texture(ColorFormat format, const Resolution &imageSize) const
{
   auto vulkanColorFormat = *vulkan::to_vulkan_color_format(format);

   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.format = vulkanColorFormat;
   imageInfo.extent = VkExtent3D{imageSize.width, imageSize.height, 1};
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.mipLevels = 1;
   imageInfo.arrayLayers = 1;
   imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

   vulkan::Image image(*m_device);
   if (image.construct(&imageInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   VkMemoryRequirements memRequirements;
   vkGetImageMemoryRequirements(*m_device, *image, &memRequirements);

   VkMemoryAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize = memRequirements.size;
   allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

   vulkan::DeviceMemory imageMemory(*m_device);
   if (imageMemory.construct(&allocInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   vkBindImageMemory(*m_device, *image, *imageMemory, 0);

   VkImageViewCreateInfo imageViewInfo{};
   imageViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   imageViewInfo.image                           = *image;
   imageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
   imageViewInfo.format                          = vulkanColorFormat;
   imageViewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
   imageViewInfo.subresourceRange.baseMipLevel   = 0;
   imageViewInfo.subresourceRange.levelCount     = 1;
   imageViewInfo.subresourceRange.baseArrayLayer = 0;
   imageViewInfo.subresourceRange.layerCount     = 1;

   vulkan::ImageView imageView(*m_device);
   if (imageView.construct(&imageViewInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return Texture(std::move(image), std::move(imageMemory), std::move(imageView), imageSize.width, imageSize.height);
}

constexpr std::array g_vulkanInstanceExtensions
{
   VK_KHR_SURFACE_EXTENSION_NAME,
#if GAPI_PLATFORM_WINDOWS
           VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif GAPI_PLATFORM_WAYLAND
           VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
#if GAPI_ENABLE_VALIDATION
           VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

constexpr std::array g_vulkanDeviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

constexpr std::array g_vulkanInstanceLayers
{
#if GAPI_ENABLE_VALIDATION
   "VK_LAYER_KHRONOS_validation",
#endif
};

Result<DeviceUPtr> initialize_device(const Surface &surface)
{
   VkApplicationInfo appInfo{};
   appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName   = "Hello Triangle";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName        = "No Engine";
   appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion         = VK_API_VERSION_1_3;

   VkInstanceCreateInfo instanceInfo{};
   instanceInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   instanceInfo.pApplicationInfo        = &appInfo;
   instanceInfo.enabledLayerCount       = 0;
   instanceInfo.ppEnabledLayerNames     = nullptr;
   instanceInfo.enabledExtensionCount   = g_vulkanInstanceExtensions.size();
   instanceInfo.ppEnabledExtensionNames = g_vulkanInstanceExtensions.data();
   instanceInfo.enabledLayerCount       = g_vulkanInstanceLayers.size();
   instanceInfo.ppEnabledLayerNames     = g_vulkanInstanceLayers.data();

   vulkan::Instance instance;
   if (const auto res = instance.construct(&instanceInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

#if GAPI_ENABLE_VALIDATION
   VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
   debugMessengerInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debugMessengerInfo.pfnUserCallback = validation_layers_callback;
   debugMessengerInfo.pUserData       = nullptr;

   vulkan::DebugUtilsMessengerEXT debugMessenger(*instance);
   if (const auto res = debugMessenger.construct(&debugMessengerInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }
#endif

   auto physicalDevices = vulkan::get_physical_devices(*instance);
   auto pickedDevice =
           std::find_if(physicalDevices.begin(), physicalDevices.end(), physical_device_pick_predicate);
   if (pickedDevice == physicalDevices.end()) {
      pickedDevice = physicalDevices.begin();
   }

   vulkan::SurfaceKHR vulkan_surface(*instance);
   if (const auto res = vulkan::to_vulkan_surface(vulkan_surface, surface); res != Status::Success)
      return std::unexpected(res);

   auto queueFamilies = vulkan::get_queue_family_properties(*pickedDevice);

   auto graphicsQueue = std::find_if(queueFamilies.begin(), queueFamilies.end(), graphics_family_predicate);
   if (graphicsQueue == queueFamilies.end()) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   vulkan::QueueFamilyIndices queueFamilyIndices{
           .graphicsQueue = static_cast<uint32_t>(graphicsQueue - queueFamilies.begin()),
   };

   for (uint32_t i{0}; i < queueFamilies.size(); ++i) {
      VkBool32 supported;
      if (vkGetPhysicalDeviceSurfaceSupportKHR(*pickedDevice, i, *vulkan_surface, &supported) != VK_SUCCESS)
         return std::unexpected(Status::UnsupportedDevice);

      if (supported) {
         queueFamilyIndices.presentQueue = i;
         if (i != queueFamilyIndices.graphicsQueue) {
            break;
         }
      }
   }

   std::set<uint32_t> uniqueFamilies;
   uniqueFamilies.emplace(queueFamilyIndices.graphicsQueue);
   uniqueFamilies.emplace(queueFamilyIndices.presentQueue);

   std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
   deviceQueueCreateInfos.resize(uniqueFamilies.size());

   uint32_t maxQueues = 0;
   for (const auto index : uniqueFamilies) {
      const auto queueCount = queueFamilies[index].queueCount;
      if (queueCount > maxQueues) {
         maxQueues = queueCount;
      }
   }

   std::vector<float> queuePriorities{};
   queuePriorities.resize(maxQueues);
   std::ranges::fill(queuePriorities, 1.0f);

   std::transform(uniqueFamilies.begin(), uniqueFamilies.end(), deviceQueueCreateInfos.begin(),
                  [&queuePriorities, &queueFamilies](uint32_t index) {
                     VkDeviceQueueCreateInfo info{};
                     info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                     info.queueCount       = queueFamilies[index].queueCount;
                     info.queueFamilyIndex = index;
                     info.pQueuePriorities = queuePriorities.data();
                     return info;
                  });

   VkPhysicalDeviceFeatures deviceFeatures{};

   VkDeviceCreateInfo deviceInfo{};
   deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   deviceInfo.queueCreateInfoCount    = static_cast<uint32_t>(deviceQueueCreateInfos.size());
   deviceInfo.pQueueCreateInfos       = deviceQueueCreateInfos.data();
   deviceInfo.pEnabledFeatures        = &deviceFeatures;
   deviceInfo.enabledExtensionCount   = g_vulkanDeviceExtensions.size();
   deviceInfo.ppEnabledExtensionNames = g_vulkanDeviceExtensions.data();

   vulkan::Device device;
   if (const auto res = device.construct(*pickedDevice, &deviceInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   VkCommandPoolCreateInfo commandPoolInfo{};
   commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   commandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsQueue;

   vulkan::CommandPool commandPool(*device);
   if (commandPool.construct(&commandPoolInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return std::make_unique<Device>(std::move(instance),
#if GAPI_ENABLE_VALIDATION
                                   std::move(debugMessenger),
#endif
                                   std::move(vulkan_surface), std::move(device), *pickedDevice,
                                   queueFamilyIndices, std::move(commandPool));
}

}// namespace graphics_api
