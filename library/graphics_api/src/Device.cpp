#include "Device.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <format>
#include <shared_mutex>
#include <vector>

#include "CommandList.hpp"
#include "Surface.hpp"
#include "vulkan/DynamicProcedures.hpp"
#include "vulkan/Util.hpp"

#undef max

namespace triglav::graphics_api {

DECLARE_VLK_ENUMERATOR(get_surface_formats, VkSurfaceFormatKHR, vkGetPhysicalDeviceSurfaceFormatsKHR)
DECLARE_VLK_ENUMERATOR(get_swapchain_images, VkImage, vkGetSwapchainImagesKHR)

namespace {

bool is_surface_format_supported(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface, const ColorFormat& colorFormat,
                                 const ColorSpace colorSpace)
{
   const auto formats = vulkan::get_surface_formats(physicalDevice, surface);

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

uint32_t swapchain_image_count(const uint32_t min, const uint32_t max)
{
   if (max == 0) {
      return std::max(2u, min);
   }

   return std::min(std::max(2u, min), max);
}

}// namespace

Device::Device(vulkan::Device device, const VkPhysicalDevice physicalDevice, std::vector<QueueFamilyInfo>&& queueFamilyInfos,
               const DeviceFeatureFlags enabledFeatures) :
    m_device(std::move(device)),
    m_physicalDevice(physicalDevice),
    m_queueFamilyInfos{std::move(queueFamilyInfos)},
    m_enabledFeatures{enabledFeatures},
    m_queueManager(*this, m_queueFamilyInfos),
    m_samplerCache(*this)
{
   vulkan::DynamicProcedures::the().init(*m_device);
}

Result<Swapchain> Device::create_swapchain(const Surface& surface, ColorFormat colorFormat, ColorSpace colorSpace,
                                           const Resolution& resolution, PresentMode presentMode, Swapchain* oldSwapchain)
{
   if (not is_surface_format_supported(m_physicalDevice, surface.vulkan_surface(), colorFormat, colorSpace))
      return std::unexpected(Status::UnsupportedFormat);

   const auto vulkanColorFormat = vulkan::to_vulkan_color_format(colorFormat);
   if (not vulkanColorFormat.has_value())
      return std::unexpected(Status::UnsupportedFormat);

   const auto vulkanColorSpace = vulkan::to_vulkan_color_space(colorSpace);
   if (not vulkanColorSpace.has_value())
      return std::unexpected(Status::UnsupportedFormat);

   VkSurfaceCapabilitiesKHR capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface.vulkan_surface(), &capabilities);

   VkSwapchainCreateInfoKHR swapchainInfo{};
   swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   swapchainInfo.surface = surface.vulkan_surface();
   swapchainInfo.presentMode = vulkan::to_vulkan_present_mode(presentMode);
   swapchainInfo.imageExtent = VkExtent2D{resolution.width, resolution.height};
   swapchainInfo.imageFormat = *vulkanColorFormat;
   swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
   if (oldSwapchain != nullptr) {
      swapchainInfo.oldSwapchain = oldSwapchain->vulkan_swapchain();
   }
   swapchainInfo.minImageCount = swapchain_image_count(capabilities.minImageCount, capabilities.maxImageCount);
   swapchainInfo.imageColorSpace = *vulkanColorSpace;
   swapchainInfo.imageArrayLayers = 1;
   swapchainInfo.preTransform = capabilities.currentTransform;
   swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   swapchainInfo.clipped = true;

   const std::array queueFamilyIndices{
      m_queueManager.queue_index(WorkType::Graphics),
      m_queueManager.queue_index(WorkType::Presentation),
   };
   if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
      swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapchainInfo.queueFamilyIndexCount = queueFamilyIndices.size();
      swapchainInfo.pQueueFamilyIndices = queueFamilyIndices.data();
   } else {
      swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   }

   vulkan::SwapchainKHR swapchain(*m_device);
   if (swapchain.construct(&swapchainInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedFormat);
   }


   const auto images = vulkan::get_swapchain_images(*m_device, *swapchain);

   std::vector<Texture> swapchainTextures;
   swapchainTextures.reserve(images.size());

   for (const auto image : images) {
      VkImageViewCreateInfo imageViewInfo{};
      imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageViewInfo.image = image;
      imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageViewInfo.format = *vulkanColorFormat;
      imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageViewInfo.subresourceRange.baseMipLevel = 0;
      imageViewInfo.subresourceRange.levelCount = 1;
      imageViewInfo.subresourceRange.baseArrayLayer = 0;
      imageViewInfo.subresourceRange.layerCount = 1;

      vulkan::ImageView imageView(*m_device);
      if (imageView.construct(&imageViewInfo) != VK_SUCCESS) {
         return std::unexpected(Status::UnsupportedDevice);
      }

      swapchainTextures.emplace_back(image, std::move(imageView), colorFormat, TextureUsage::ColorAttachment | TextureUsage::TransferDst,
                                     resolution.width, resolution.height, 1);
   }

   return Swapchain(m_queueManager, resolution, std::move(swapchainTextures), std::move(swapchain), colorFormat);
}

Result<Shader> Device::create_shader(const PipelineStage stage, const std::string_view entrypoint, const std::span<const char> code)
{
   VkShaderModuleCreateInfo shaderModuleInfo{};
   shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   shaderModuleInfo.codeSize = code.size();
   shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

   vulkan::ShaderModule shaderModule(*m_device);
   if (shaderModule.construct(&shaderModuleInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Shader(std::string{entrypoint}, stage, std::move(shaderModule));
}

Result<CommandList> Device::create_command_list(const WorkTypeFlags flags) const
{
   return m_queueManager.create_command_list(flags);
}

Result<DescriptorPool> Device::create_descriptor_pool(std::span<const std::pair<DescriptorType, u32>> descriptorCounts,
                                                      const u32 maxDescriptorCount)
{
   std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
   descriptorPoolSizes.reserve(descriptorCounts.size());
   for (const auto& [descType, count] : descriptorCounts) {
      descriptorPoolSizes.emplace_back(vulkan::to_vulkan_descriptor_type(descType), count);
   }

   VkDescriptorPoolCreateInfo descriptorPoolInfo{};
   descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
   descriptorPoolInfo.poolSizeCount = descriptorPoolSizes.size();
   descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
   descriptorPoolInfo.maxSets = maxDescriptorCount;

   vulkan::DescriptorPool descriptorPool{*m_device};
   if (descriptorPool.construct(&descriptorPoolInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return DescriptorPool(std::move(descriptorPool));
}

Result<Buffer> Device::create_buffer(const BufferUsageFlags usage, const uint64_t size)
{
   assert(size != 0);
   assert(usage != BufferUsage::None);

   VkBufferCreateInfo bufferInfo{};
   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size = size;
   bufferInfo.usage = vulkan::to_vulkan_buffer_usage_flags(usage);
   bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

   vulkan::Buffer buffer(*m_device);
   if (const auto res = buffer.construct(&bufferInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   VkMemoryRequirements memRequirements;
   vkGetBufferMemoryRequirements(*m_device, *buffer, &memRequirements);

   VkMemoryAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
   allocateInfo.allocationSize = memRequirements.size;
   allocateInfo.memoryTypeIndex = this->find_memory_type(memRequirements.memoryTypeBits, vulkan::to_vulkan_memory_properties_flags(usage));

   VkMemoryAllocateFlagsInfo allocateFlagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
   if (!(usage & BufferUsage::HostVisible)) {
      allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
      allocateInfo.pNext = &allocateFlagsInfo;
   }

   vulkan::DeviceMemory memory(*m_device);
   if (memory.construct(&allocateInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   if (vkBindBufferMemory(*m_device, *buffer, *memory, 0) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Buffer{*this, size, std::move(buffer), std::move(memory)};
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

Result<Texture> Device::create_texture_from_ktx(const ktx::Texture& texture, const TextureUsageFlags usageFlags,
                                                const TextureState finalState)
{
   const auto vkImageUsage = vulkan::to_vulkan_image_usage_flags(usageFlags);

   std::optional<ktx::VulkanTexture> texProps;
   {
      // need to access the queue to be able to access texture
      auto [queueAccess, ktxDevice] = m_queueManager.ktx_device_info();
      auto accessor{queueAccess.access()};

      texProps = ktxDevice.upload_texture(texture, VK_IMAGE_TILING_OPTIMAL, vkImageUsage,
                                          vulkan::to_vulkan_image_layout(GAPI_FORMAT(RGBA, sRGB), finalState));
   }

   if (!texProps.has_value()) {
      return std::unexpected{Status::UnsupportedDevice};
   }

   vulkan::Image wrappedImage(*m_device);
   wrappedImage.take_ownership(texProps->image);

   vulkan::DeviceMemory wrappedMemory(*m_device);
   wrappedMemory.take_ownership(texProps->memory);

   VkImageViewCreateInfo imageViewInfo{};
   imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   imageViewInfo.image = texProps->image;
   imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   imageViewInfo.format = texProps->format;
   imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.subresourceRange.aspectMask = vulkan::to_vulkan_image_aspect_flags(usageFlags);
   imageViewInfo.subresourceRange.baseMipLevel = 0;
   imageViewInfo.subresourceRange.levelCount = texProps->mipCount;
   imageViewInfo.subresourceRange.baseArrayLayer = 0;
   imageViewInfo.subresourceRange.layerCount = 1;

   vulkan::ImageView imageView(*m_device);
   if (imageView.construct(&imageViewInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return Texture(std::move(wrappedImage), std::move(wrappedMemory), std::move(imageView),
                  GAPI_CHECK(vulkan::to_color_format(texProps->format)), usageFlags, texProps->imageSize.x, texProps->imageSize.y,
                  static_cast<int>(texProps->mipCount));
}

Result<Texture> Device::create_texture(const ColorFormat& format, const Resolution& imageSize, const TextureUsageFlags usageFlags,
                                       const TextureState initialTextureState, const SampleCount sampleCount, int mipCount) const
{
   assert(usageFlags != TextureUsage::None);

   const auto vulkanColorFormat = *vulkan::to_vulkan_color_format(format);

   if (mipCount == 0) {
      mipCount = static_cast<int>(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
   }

   VkImageFormatProperties formatProperties;
   if (const auto res =
          vkGetPhysicalDeviceImageFormatProperties(m_physicalDevice, vulkanColorFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                                                   vulkan::to_vulkan_image_usage_flags(usageFlags), 0, &formatProperties);
       res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedFormat);
   }

   if (mipCount > static_cast<int>(formatProperties.maxMipLevels)) {
      mipCount = static_cast<int>(formatProperties.maxMipLevels);
   }

   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.format = vulkanColorFormat;
   imageInfo.extent = VkExtent3D{imageSize.width, imageSize.height, 1};
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.mipLevels = mipCount;
   imageInfo.arrayLayers = 1;
   imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
   imageInfo.initialLayout = vulkan::to_vulkan_image_layout(format, initialTextureState);
   imageInfo.samples = static_cast<VkSampleCountFlagBits>(sampleCount);
   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   imageInfo.usage = vulkan::to_vulkan_image_usage_flags(usageFlags);

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
   imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   imageViewInfo.image = *image;
   imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   imageViewInfo.format = vulkanColorFormat;
   imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.subresourceRange.aspectMask = vulkan::to_vulkan_image_aspect_flags(usageFlags);
   imageViewInfo.subresourceRange.baseMipLevel = 0;
   imageViewInfo.subresourceRange.levelCount = mipCount;
   imageViewInfo.subresourceRange.baseArrayLayer = 0;
   imageViewInfo.subresourceRange.layerCount = 1;

   vulkan::ImageView imageView(*m_device);
   if (imageView.construct(&imageViewInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return Texture(std::move(image), std::move(imageMemory), std::move(imageView), format, usageFlags, imageSize.width, imageSize.height,
                  mipCount);
}

Result<Sampler> Device::create_sampler(const SamplerProperties& info)
{
   VkPhysicalDeviceProperties properties{};
   vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

   VkSamplerCreateInfo samplerInfo{};
   samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   samplerInfo.magFilter = vulkan::to_vulkan_filter(info.magFilter);
   samplerInfo.minFilter = vulkan::to_vulkan_filter(info.minFilter);
   samplerInfo.addressModeU = vulkan::to_vulkan_sampler_address_mode(info.addressU);
   samplerInfo.addressModeV = vulkan::to_vulkan_sampler_address_mode(info.addressV);
   samplerInfo.addressModeW = vulkan::to_vulkan_sampler_address_mode(info.addressW);
   samplerInfo.anisotropyEnable = info.enableAnisotropy;
   samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
   samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   samplerInfo.unnormalizedCoordinates = false;
   samplerInfo.compareEnable = false;
   samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
   samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   samplerInfo.minLod = info.minLod;
   samplerInfo.maxLod = info.maxLod;
   samplerInfo.mipLodBias = info.mipBias;

   vulkan::Sampler sampler(*m_device);
   if (sampler.construct(&samplerInfo) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Sampler(std::move(sampler));
}

Result<QueryPool> Device::create_query_pool(const QueryType queryType, const u32 timestampCount)
{
   VkQueryPoolCreateInfo queryPoolInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
   queryPoolInfo.queryType = vulkan::to_vulkan_query_type(queryType);
   queryPoolInfo.queryCount = timestampCount;
   if (queryType == QueryType::PipelineStats) {
      queryPoolInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;
   }

   VkPhysicalDeviceProperties properties;
   vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

   vulkan::QueryPool queryPool(*m_device);
   if (const auto res = queryPool.construct(&queryPoolInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return QueryPool(std::move(queryPool), properties.limits.timestampPeriod);
}

std::pair<Resolution, Resolution> Device::get_surface_resolution_limits(const Surface& surface) const
{
   VkSurfaceCapabilitiesKHR capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface.vulkan_surface(), &capabilities);

   return {Resolution{capabilities.minImageExtent.width, capabilities.minImageExtent.height},
           Resolution{capabilities.maxImageExtent.width, capabilities.maxImageExtent.height}};
}

Status Device::submit_command_list(const CommandList& commandList, const SemaphoreArrayView waitSemaphores,
                                   const SemaphoreArrayView signalSemaphores, const Fence* fence, const WorkTypeFlags /*workTypes*/)
{
   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   std::vector<VkPipelineStageFlags> waitStages{};
   waitStages.resize(waitSemaphores.semaphore_count());
   // std::ranges::fill(waitStages, vulkan::to_vulkan_wait_pipeline_stage(workTypes));
   std::ranges::fill(waitStages, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
   const std::array commandBuffers{commandList.vulkan_command_buffer()};

   submitInfo.waitSemaphoreCount = waitSemaphores.semaphore_count();
   submitInfo.pWaitSemaphores = waitSemaphores.vulkan_semaphores();
   submitInfo.pWaitDstStageMask = waitStages.data();
   submitInfo.commandBufferCount = commandBuffers.size();
   submitInfo.pCommandBuffers = commandBuffers.data();
   submitInfo.signalSemaphoreCount = signalSemaphores.semaphore_count();
   submitInfo.pSignalSemaphores = signalSemaphores.vulkan_semaphores();

   VkFence vulkanFence{};
   if (fence != nullptr) {
      vulkanFence = fence->vulkan_fence();
   }

   auto& queue = m_queueManager.next_queue(commandList.work_types());
   auto queueAccessor = queue.access();

   if (vkQueueSubmit(*queueAccessor, 1, &submitInfo, vulkanFence) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status Device::submit_command_list(const CommandList& commandList, const Semaphore& waitSemaphore, const Semaphore& signalSemaphore,
                                   const Fence& fence)
{
   SemaphoreArray waitSemaphores;
   waitSemaphores.add_semaphore(waitSemaphore);
   SemaphoreArray signalSemaphores;
   signalSemaphores.add_semaphore(signalSemaphore);

   return this->submit_command_list(commandList, waitSemaphores, signalSemaphores, &fence, WorkType::Graphics);
}

Status Device::submit_command_list_one_time(const CommandList& commandList)
{
   const auto vulkanCommandList = commandList.vulkan_command_buffer();

   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &vulkanCommandList;

   auto& queue = m_queueManager.next_queue(commandList.work_types());
   auto queueAccessor = queue.access();

   if (vkQueueSubmit(*queueAccessor, 1, &submitInfo, nullptr) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   if (vkQueueWaitIdle(*queueAccessor) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

VkDevice Device::vulkan_device() const
{
   return *m_device;
}

VkPhysicalDevice Device::vulkan_physical_device() const
{
   return m_physicalDevice;
}

QueueManager& Device::queue_manager()
{
   return m_queueManager;
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
      if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
         return i;
      }
   }

   return 0;
}

SamplerCache& Device::sampler_cache()
{
   return m_samplerCache;
}

DeviceFeatureFlags Device::enabled_features() const
{
   return m_enabledFeatures;
}

Result<ktx::Texture> Device::export_ktx_texture(const Texture& texture)
{
   if (texture.format() != GAPI_FORMAT(RGBA, sRGB) && texture.format() != GAPI_FORMAT(RGBA, UNorm8)) {
      return std::unexpected{Status::UnsupportedFormat};
   }

   ktx::TextureCreateInfo createInfo{};
   createInfo.dimensions = {texture.resolution().width, texture.resolution().height};
   createInfo.format = texture.format() == GAPI_FORMAT(RGBA, sRGB) ? ktx::Format::R8G8B8A8_SRGB : ktx::Format::R8G8B8A8_UNORM;
   createInfo.generateMipmaps = false;
   createInfo.createMipLayers = texture.mip_count() > 1;
   auto ktxTex = ktx::Texture::create(createInfo);
   if (!ktxTex.has_value()) {
      return std::unexpected{Status::UnsupportedFormat};
   }

   u32 width = texture.resolution().width;
   u32 height = texture.resolution().height;
   for (u32 mipLevel = 0; mipLevel < texture.mip_count(); mipLevel++) {
      const auto bufferSize = width * height * sizeof(u32);
      auto buffer = this->create_buffer(BufferUsage::TransferDst | BufferUsage::HostVisible, bufferSize);
      if (!buffer.has_value()) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      auto cmdList = this->create_command_list(WorkType::Compute);
      if (!cmdList.has_value()) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      if (cmdList->begin(SubmitType::OneTime) != Status::Success) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      TextureBarrierInfo inBarrier{};
      inBarrier.texture = &texture;
      inBarrier.sourceState = TextureState::Undefined;
      inBarrier.targetState = TextureState::TransferSrc;
      inBarrier.baseMipLevel = static_cast<int>(mipLevel);
      inBarrier.mipLevelCount = 1;
      cmdList->texture_barrier(PipelineStage::Entrypoint, PipelineStage::Transfer, {&inBarrier, 1});

      cmdList->copy_texture_to_buffer(texture, *buffer, static_cast<int>(mipLevel));
      if (cmdList->finish() != Status::Success) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      if (this->submit_command_list_one_time(*cmdList) != Status::Success) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      auto mem = buffer->map_memory();
      if (!mem.has_value()) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      if (!ktxTex->set_image_from_buffer({static_cast<const u8*>(**mem), bufferSize}, mipLevel, 0, 0)) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      width /= 2;
      height /= 2;

      if (width == 0) {
         width = 1;
      }
      if (height == 0) {
         height = 1;
      }
   }

   return std::move(*ktxTex);
}

u32 Device::min_storage_buffer_alignment() const
{
   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
   return props.limits.minStorageBufferOffsetAlignment;
}

Result<ray_tracing::AccelerationStructure> Device::create_acceleration_structure(const ray_tracing::AccelerationStructureType structType,
                                                                                 const Buffer& buffer, const MemorySize bufferOffset,
                                                                                 const MemorySize bufferSize)
{
   VkAccelerationStructureCreateInfoKHR asInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
   asInfo.buffer = buffer.vulkan_buffer();
   asInfo.offset = bufferOffset;
   asInfo.size = std::min(buffer.size() - bufferOffset, bufferSize);
   asInfo.type = vulkan::to_vulkan_acceleration_structure_type(structType);

   vulkan::AccelerationStructureKHR structure(*m_device);
   if (structure.construct(&asInfo) != VK_SUCCESS) {
      return std::unexpected{Status::UnsupportedDevice};
   }

   return ray_tracing::AccelerationStructure(std::move(structure));
}

}// namespace triglav::graphics_api
