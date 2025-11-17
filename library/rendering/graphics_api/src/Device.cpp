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

bool is_surface_format_supported(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface, const ColorFormat& color_format,
                                 const ColorSpace color_space)
{
   const auto formats = vulkan::get_surface_formats(physical_device, surface);

   const auto target_vulkan_format = vulkan::to_vulkan_color_format(color_format);
   if (not target_vulkan_format.has_value()) {
      return false;
   }

   const auto target_vulkan_color_space = vulkan::to_vulkan_color_space(color_space);
   if (not target_vulkan_color_space.has_value()) {
      return false;
   }

   for (const auto [vulkan_format, vulkan_color_space] : formats) {
      if (vulkan_format != target_vulkan_format)
         continue;
      if (vulkan_color_space != target_vulkan_color_space)
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

Device::Device(vulkan::Device device, const VkPhysicalDevice physical_device, std::vector<QueueFamilyInfo>&& queue_family_infos,
               const DeviceFeatureFlags enabled_features) :
    m_device(std::move(device)),
    m_physical_device(physical_device),
    m_queue_family_infos{std::move(queue_family_infos)},
    m_enabled_features{enabled_features},
    m_queue_manager(*this, m_queue_family_infos),
    m_sampler_cache(*this)
{
   vulkan::DynamicProcedures::the().init(*m_device);
}

Result<Swapchain> Device::create_swapchain(const Surface& surface, ColorFormat color_format, ColorSpace color_space,
                                           const Resolution& resolution, PresentMode present_mode, Swapchain* old_swapchain)
{
   if (not is_surface_format_supported(m_physical_device, surface.vulkan_surface(), color_format, color_space))
      return std::unexpected(Status::UnsupportedFormat);

   const auto vulkan_color_format = vulkan::to_vulkan_color_format(color_format);
   if (not vulkan_color_format.has_value())
      return std::unexpected(Status::UnsupportedFormat);

   const auto vulkan_color_space = vulkan::to_vulkan_color_space(color_space);
   if (not vulkan_color_space.has_value())
      return std::unexpected(Status::UnsupportedFormat);

   VkSurfaceCapabilitiesKHR capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, surface.vulkan_surface(), &capabilities);

   VkSwapchainCreateInfoKHR swapchain_info{};
   swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   swapchain_info.surface = surface.vulkan_surface();
   swapchain_info.presentMode = vulkan::to_vulkan_present_mode(present_mode);
   swapchain_info.imageExtent = VkExtent2D{resolution.width, resolution.height};
   swapchain_info.imageFormat = *vulkan_color_format;
   swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
   if (old_swapchain != nullptr) {
      swapchain_info.oldSwapchain = old_swapchain->vulkan_swapchain();
   }
   swapchain_info.minImageCount = swapchain_image_count(capabilities.minImageCount, capabilities.maxImageCount);
   swapchain_info.imageColorSpace = *vulkan_color_space;
   swapchain_info.imageArrayLayers = 1;
   swapchain_info.preTransform = capabilities.currentTransform;
   swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   swapchain_info.clipped = true;

   const std::array queue_family_indices{
      m_queue_manager.queue_index(WorkType::Graphics),
      m_queue_manager.queue_index(WorkType::Presentation),
   };
   if (queue_family_indices[0] != queue_family_indices[1]) {
      swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapchain_info.queueFamilyIndexCount = static_cast<u32>(queue_family_indices.size());
      swapchain_info.pQueueFamilyIndices = queue_family_indices.data();
   } else {
      swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   }

   vulkan::SwapchainKHR swapchain(*m_device);
   if (swapchain.construct(&swapchain_info) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedFormat);
   }


   const auto images = vulkan::get_swapchain_images(*m_device, *swapchain);

   std::vector<Texture> swapchain_textures;
   swapchain_textures.reserve(images.size());

   for (const auto image : images) {
      VkImageViewCreateInfo image_view_info{};
      image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      image_view_info.image = image;
      image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      image_view_info.format = *vulkan_color_format;
      image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_view_info.subresourceRange.baseMipLevel = 0;
      image_view_info.subresourceRange.levelCount = 1;
      image_view_info.subresourceRange.baseArrayLayer = 0;
      image_view_info.subresourceRange.layerCount = 1;

      vulkan::ImageView image_view(*m_device);
      if (image_view.construct(&image_view_info) != VK_SUCCESS) {
         return std::unexpected(Status::UnsupportedDevice);
      }

      swapchain_textures.emplace_back(image, std::move(image_view), color_format, TextureUsage::ColorAttachment | TextureUsage::TransferDst,
                                      resolution.width, resolution.height, 1);
   }

   return Swapchain(m_queue_manager, resolution, std::move(swapchain_textures), std::move(swapchain), color_format);
}

Result<Shader> Device::create_shader(const PipelineStage stage, const std::string_view entrypoint, const std::span<const char> code)
{
   VkShaderModuleCreateInfo shader_module_info{};
   shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   shader_module_info.codeSize = code.size();
   shader_module_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

   vulkan::ShaderModule shader_module(*m_device);
   if (shader_module.construct(&shader_module_info) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Shader(std::string{entrypoint}, stage, std::move(shader_module));
}

Result<CommandList> Device::create_command_list(const WorkTypeFlags flags) const
{
   return m_queue_manager.create_command_list(flags);
}

Result<DescriptorPool> Device::create_descriptor_pool(std::span<const std::pair<DescriptorType, u32>> descriptor_counts,
                                                      const u32 max_descriptor_count)
{
   std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
   descriptor_pool_sizes.reserve(descriptor_counts.size());
   for (const auto& [desc_type, count] : descriptor_counts) {
      descriptor_pool_sizes.emplace_back(vulkan::to_vulkan_descriptor_type(desc_type), count);
   }

   VkDescriptorPoolCreateInfo descriptor_pool_info{};
   descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   descriptor_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
   descriptor_pool_info.poolSizeCount = static_cast<u32>(descriptor_pool_sizes.size());
   descriptor_pool_info.pPoolSizes = descriptor_pool_sizes.data();
   descriptor_pool_info.maxSets = max_descriptor_count;

   vulkan::DescriptorPool descriptor_pool{*m_device};
   if (descriptor_pool.construct(&descriptor_pool_info) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return DescriptorPool(std::move(descriptor_pool));
}

Result<Buffer> Device::create_buffer(const BufferUsageFlags usage, const uint64_t size)
{
   assert(size != 0);
   assert(usage != BufferUsage::None);

   VkBufferCreateInfo buffer_info{};
   buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   buffer_info.size = size;
   buffer_info.usage = vulkan::to_vulkan_buffer_usage_flags(usage);
   buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

   vulkan::Buffer buffer(*m_device);
   if (const auto res = buffer.construct(&buffer_info); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   VkMemoryRequirements mem_requirements;
   vkGetBufferMemoryRequirements(*m_device, *buffer, &mem_requirements);

   VkMemoryAllocateInfo allocate_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
   allocate_info.allocationSize = mem_requirements.size;
   allocate_info.memoryTypeIndex =
      this->find_memory_type(mem_requirements.memoryTypeBits, vulkan::to_vulkan_memory_properties_flags(usage));

   VkMemoryAllocateFlagsInfo allocate_flags_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
   if (!(usage & BufferUsage::HostVisible)) {
      allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
      allocate_info.pNext = &allocate_flags_info;
   }

   vulkan::DeviceMemory memory(*m_device);
   if (memory.construct(&allocate_info) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   if (vkBindBufferMemory(*m_device, *buffer, *memory, 0) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Buffer{*this, size, std::move(buffer), std::move(memory)};
}

Result<Fence> Device::create_fence() const
{
   VkFenceCreateInfo fence_info{};
   fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

   vulkan::Fence fence(*m_device);
   if (fence.construct(&fence_info) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Fence(std::move(fence));
}

Result<Semaphore> Device::create_semaphore() const
{
   VkSemaphoreCreateInfo semaphore_info{};
   semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   vulkan::Semaphore semaphore(*m_device);
   if (semaphore.construct(&semaphore_info) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Semaphore(std::move(semaphore));
}

Result<Texture> Device::create_texture_from_ktx(const ktx::Texture& texture, const TextureUsageFlags usage_flags,
                                                const TextureState final_state)
{
   const auto vkImageUsage = vulkan::to_vulkan_image_usage_flags(usage_flags);

   std::optional<ktx::VulkanTexture> tex_props;
   {
      // need to access the queue to be able to access texture
      auto [queue_access, ktxDevice] = m_queue_manager.ktx_device_info();
      auto accessor{queue_access.access()};

      tex_props = ktxDevice.upload_texture(texture, VK_IMAGE_TILING_OPTIMAL, vkImageUsage,
                                           vulkan::to_vulkan_image_layout(GAPI_FORMAT(RGBA, sRGB), final_state));
   }

   if (!tex_props.has_value()) {
      return std::unexpected{Status::UnsupportedDevice};
   }

   vulkan::Image wrapped_image(*m_device);
   wrapped_image.take_ownership(tex_props->image);

   vulkan::DeviceMemory wrapped_memory(*m_device);
   wrapped_memory.take_ownership(tex_props->memory);

   VkImageViewCreateInfo image_view_info{};
   image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   image_view_info.image = tex_props->image;
   image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   image_view_info.format = tex_props->format;
   image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.subresourceRange.aspectMask = vulkan::to_vulkan_image_aspect_flags(usage_flags);
   image_view_info.subresourceRange.baseMipLevel = 0;
   image_view_info.subresourceRange.levelCount = tex_props->mip_count;
   image_view_info.subresourceRange.baseArrayLayer = 0;
   image_view_info.subresourceRange.layerCount = 1;

   vulkan::ImageView image_view(*m_device);
   if (image_view.construct(&image_view_info) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return Texture(std::move(wrapped_image), std::move(wrapped_memory), std::move(image_view),
                  GAPI_CHECK(vulkan::to_color_format(tex_props->format)), usage_flags, tex_props->image_size.x, tex_props->image_size.y,
                  static_cast<int>(tex_props->mip_count));
}

Result<Texture> Device::create_texture(const ColorFormat& format, const Resolution& image_size, const TextureUsageFlags usage_flags,
                                       const TextureState initial_texture_state, const SampleCount sample_count, int mip_count) const
{
   assert(usage_flags != TextureUsage::None);

   const auto vulkan_color_format = *vulkan::to_vulkan_color_format(format);

   if (mip_count == 0) {
      mip_count = static_cast<int>(std::floor(std::log2(std::max(image_size.width, image_size.height)))) + 1;
   }

   VkImageFormatProperties format_properties;
   if (const auto res =
          vkGetPhysicalDeviceImageFormatProperties(m_physical_device, vulkan_color_format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                                                   vulkan::to_vulkan_image_usage_flags(usage_flags), 0, &format_properties);
       res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedFormat);
   }

   if (mip_count > static_cast<int>(format_properties.maxMipLevels)) {
      mip_count = static_cast<int>(format_properties.maxMipLevels);
   }

   VkImageCreateInfo image_info{};
   image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   image_info.format = vulkan_color_format;
   image_info.extent = VkExtent3D{image_size.width, image_size.height, 1};
   image_info.imageType = VK_IMAGE_TYPE_2D;
   image_info.mipLevels = mip_count;
   image_info.arrayLayers = 1;
   image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
   image_info.initialLayout = vulkan::to_vulkan_image_layout(format, initial_texture_state);
   image_info.samples = static_cast<VkSampleCountFlagBits>(sample_count);
   image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   image_info.usage = vulkan::to_vulkan_image_usage_flags(usage_flags);

   vulkan::Image image(*m_device);
   if (image.construct(&image_info) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   VkMemoryRequirements mem_requirements;
   vkGetImageMemoryRequirements(*m_device, *image, &mem_requirements);

   VkMemoryAllocateInfo alloc_info{};
   alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   alloc_info.allocationSize = mem_requirements.size;
   alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

   vulkan::DeviceMemory image_memory(*m_device);
   if (image_memory.construct(&alloc_info) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   vkBindImageMemory(*m_device, *image, *image_memory, 0);

   VkImageViewCreateInfo image_view_info{};
   image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   image_view_info.image = *image;
   image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   image_view_info.format = vulkan_color_format;
   image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.subresourceRange.aspectMask = vulkan::to_vulkan_image_aspect_flags(usage_flags);
   image_view_info.subresourceRange.baseMipLevel = 0;
   image_view_info.subresourceRange.levelCount = mip_count;
   image_view_info.subresourceRange.baseArrayLayer = 0;
   image_view_info.subresourceRange.layerCount = 1;

   vulkan::ImageView image_view(*m_device);
   if (image_view.construct(&image_view_info) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return Texture(std::move(image), std::move(image_memory), std::move(image_view), format, usage_flags, image_size.width,
                  image_size.height, mip_count);
}

Result<Sampler> Device::create_sampler(const SamplerProperties& info)
{
   VkPhysicalDeviceProperties properties{};
   vkGetPhysicalDeviceProperties(m_physical_device, &properties);

   VkSamplerCreateInfo sampler_info{};
   sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   sampler_info.magFilter = vulkan::to_vulkan_filter(info.mag_filter);
   sampler_info.minFilter = vulkan::to_vulkan_filter(info.min_filter);
   sampler_info.addressModeU = vulkan::to_vulkan_sampler_address_mode(info.address_u);
   sampler_info.addressModeV = vulkan::to_vulkan_sampler_address_mode(info.address_v);
   sampler_info.addressModeW = vulkan::to_vulkan_sampler_address_mode(info.address_w);
   sampler_info.anisotropyEnable = info.enable_anisotropy;
   sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
   sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   sampler_info.unnormalizedCoordinates = false;
   sampler_info.compareEnable = false;
   sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
   sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   sampler_info.minLod = info.min_lod;
   sampler_info.maxLod = info.max_lod;
   sampler_info.mipLodBias = info.mip_bias;

   vulkan::Sampler sampler(*m_device);
   if (sampler.construct(&sampler_info) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Sampler(std::move(sampler));
}

Result<QueryPool> Device::create_query_pool(const QueryType query_type, const u32 timestamp_count)
{
   VkQueryPoolCreateInfo query_pool_info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
   query_pool_info.queryType = vulkan::to_vulkan_query_type(query_type);
   query_pool_info.queryCount = timestamp_count;
   if (query_type == QueryType::PipelineStats) {
      query_pool_info.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;
   }

   VkPhysicalDeviceProperties properties;
   vkGetPhysicalDeviceProperties(m_physical_device, &properties);

   vulkan::QueryPool query_pool(*m_device);
   if (const auto res = query_pool.construct(&query_pool_info); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return QueryPool(std::move(query_pool), properties.limits.timestampPeriod);
}

std::pair<Resolution, Resolution> Device::get_surface_resolution_limits(const Surface& surface) const
{
   VkSurfaceCapabilitiesKHR capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, surface.vulkan_surface(), &capabilities);

   return {Resolution{capabilities.minImageExtent.width, capabilities.minImageExtent.height},
           Resolution{capabilities.maxImageExtent.width, capabilities.maxImageExtent.height}};
}

Vector2u Device::get_current_surface_extent(const Surface& surface) const
{
   VkSurfaceCapabilitiesKHR capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, surface.vulkan_surface(), &capabilities);
   return {capabilities.currentExtent.width, capabilities.currentExtent.height};
}

Status Device::submit_command_list(const CommandList& command_list, const SemaphoreArrayView wait_semaphores,
                                   const SemaphoreArrayView signal_semaphores, const Fence* fence, const WorkTypeFlags /*work_types*/)
{
   VkSubmitInfo submit_info{};
   submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   std::vector<VkPipelineStageFlags> wait_stages{};
   wait_stages.resize(wait_semaphores.semaphore_count());
   // std::ranges::fill(wait_stages, vulkan::to_vulkan_wait_pipeline_stage(work_types));
   std::ranges::fill(wait_stages, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
   const std::array command_buffers{command_list.vulkan_command_buffer()};

   submit_info.waitSemaphoreCount = static_cast<u32>(wait_semaphores.semaphore_count());
   submit_info.pWaitSemaphores = wait_semaphores.vulkan_semaphores();
   submit_info.pWaitDstStageMask = wait_stages.data();
   submit_info.commandBufferCount = static_cast<u32>(command_buffers.size());
   submit_info.pCommandBuffers = command_buffers.data();
   submit_info.signalSemaphoreCount = static_cast<u32>(signal_semaphores.semaphore_count());
   submit_info.pSignalSemaphores = signal_semaphores.vulkan_semaphores();

   VkFence vulkan_fence{};
   if (fence != nullptr) {
      vulkan_fence = fence->vulkan_fence();
   }

   auto& queue = m_queue_manager.next_queue(command_list.work_types());
   auto queue_accessor = queue.access();

   if (auto status = vkQueueSubmit(*queue_accessor, 1, &submit_info, vulkan_fence); status != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status Device::submit_command_list(const CommandList& command_list, const Semaphore& wait_semaphore, const Semaphore& signal_semaphore,
                                   const Fence& fence)
{
   SemaphoreArray wait_semaphores;
   wait_semaphores.add_semaphore(wait_semaphore);
   SemaphoreArray signal_semaphores;
   signal_semaphores.add_semaphore(signal_semaphore);

   return this->submit_command_list(command_list, wait_semaphores, signal_semaphores, &fence, WorkType::Graphics);
}

Status Device::submit_command_list_one_time(const CommandList& command_list)
{
   const auto vulkan_command_list = command_list.vulkan_command_buffer();

   VkSubmitInfo submit_info{};
   submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &vulkan_command_list;

   auto& queue = m_queue_manager.next_queue(command_list.work_types());
   auto queue_accessor = queue.access();

   if (vkQueueSubmit(*queue_accessor, 1, &submit_info, nullptr) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   if (vkQueueWaitIdle(*queue_accessor) != VK_SUCCESS) {
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
   return m_physical_device;
}

QueueManager& Device::queue_manager()
{
   return m_queue_manager;
}

void Device::await_all() const
{
   vkDeviceWaitIdle(*m_device);
}

uint32_t Device::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const
{
   VkPhysicalDeviceMemoryProperties mem_properties;
   vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_properties);

   for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
      if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
         return i;
      }
   }

   return 0;
}

SamplerCache& Device::sampler_cache()
{
   return m_sampler_cache;
}

DeviceFeatureFlags Device::enabled_features() const
{
   return m_enabled_features;
}

Result<ktx::Texture> Device::export_ktx_texture(const Texture& texture)
{
   if (texture.format() != GAPI_FORMAT(RGBA, sRGB) && texture.format() != GAPI_FORMAT(RGBA, UNorm8)) {
      return std::unexpected{Status::UnsupportedFormat};
   }

   ktx::TextureCreateInfo create_info{};
   create_info.dimensions = {texture.resolution().width, texture.resolution().height};
   create_info.format = texture.format() == GAPI_FORMAT(RGBA, sRGB) ? ktx::Format::R8G8B8A8_SRGB : ktx::Format::R8G8B8A8_UNORM;
   create_info.generate_mipmaps = false;
   create_info.create_mip_layers = texture.mip_count() > 1;
   auto ktxTex = ktx::Texture::create(create_info);
   if (!ktxTex.has_value()) {
      return std::unexpected{Status::UnsupportedFormat};
   }

   u32 width = texture.resolution().width;
   u32 height = texture.resolution().height;
   for (u32 mip_level = 0; mip_level < texture.mip_count(); mip_level++) {
      const auto buffer_size = width * height * sizeof(u32);
      auto buffer = this->create_buffer(BufferUsage::TransferDst | BufferUsage::HostVisible, buffer_size);
      if (!buffer.has_value()) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      auto cmd_list = this->create_command_list(WorkType::Compute);
      if (!cmd_list.has_value()) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      if (cmd_list->begin(SubmitType::OneTime) != Status::Success) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      TextureBarrierInfo in_barrier{};
      in_barrier.texture = &texture;
      in_barrier.source_state = TextureState::Undefined;
      in_barrier.target_state = TextureState::TransferSrc;
      in_barrier.base_mip_level = static_cast<int>(mip_level);
      in_barrier.mip_level_count = 1;
      cmd_list->texture_barrier(PipelineStage::Entrypoint, PipelineStage::Transfer, {&in_barrier, 1});

      cmd_list->copy_texture_to_buffer(texture, *buffer, static_cast<int>(mip_level));
      if (cmd_list->finish() != Status::Success) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      if (this->submit_command_list_one_time(*cmd_list) != Status::Success) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      auto mem = buffer->map_memory();
      if (!mem.has_value()) {
         return std::unexpected{Status::UnsupportedFormat};
      }

      if (!ktxTex->set_image_from_buffer({static_cast<const u8*>(**mem), buffer_size}, mip_level, 0, 0)) {
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

const DeviceLimits& Device::limits() const
{
   static std::optional<DeviceLimits> limits;
   if (limits.has_value()) {
      return *limits;
   }

   limits.emplace(DeviceLimits{});

   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(m_physical_device, &props);

   limits->min_uniform_buffer_alignment = props.limits.minUniformBufferOffsetAlignment;
   return *limits;
}

MemorySize Device::min_storage_buffer_alignment() const
{
   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(m_physical_device, &props);
   return props.limits.minStorageBufferOffsetAlignment;
}

Result<ray_tracing::AccelerationStructure> Device::create_acceleration_structure(const ray_tracing::AccelerationStructureType struct_type,
                                                                                 const Buffer& buffer, const MemorySize buffer_offset,
                                                                                 const MemorySize buffer_size)
{
   VkAccelerationStructureCreateInfoKHR as_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
   as_info.buffer = buffer.vulkan_buffer();
   as_info.offset = buffer_offset;
   as_info.size = std::min(buffer.size() - buffer_offset, buffer_size);
   as_info.type = vulkan::to_vulkan_acceleration_structure_type(struct_type);

   vulkan::AccelerationStructureKHR structure(*m_device);
   if (structure.construct(&as_info) != VK_SUCCESS) {
      return std::unexpected{Status::UnsupportedDevice};
   }

   return ray_tracing::AccelerationStructure(std::move(structure));
}

}// namespace triglav::graphics_api
