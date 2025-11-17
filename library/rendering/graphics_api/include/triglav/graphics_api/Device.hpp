#pragma once

#include "Buffer.hpp"
#include "GraphicsApi.hpp"
#include "QueryPool.hpp"
#include "QueueManager.hpp"
#include "Sampler.hpp"
#include "SamplerCache.hpp"
#include "Shader.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "Synchronization.hpp"
#include "Texture.hpp"
#include "ray_tracing/AccelerationStructure.hpp"
#include "ray_tracing/RayTracing.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include "triglav/ktx/Texture.hpp"

#include <memory>
#include <span>
#include <vector>

#if defined(NDEBUG) || defined(TG_DISABLE_DEBUG_UTILS)
#define GAPI_ENABLE_VALIDATION 0
#else
#define GAPI_ENABLE_VALIDATION 1
#endif

namespace triglav::desktop {
class ISurface;
}

namespace triglav::graphics_api {

class CommandList;

DECLARE_VLK_WRAPPED_OBJECT(Device)

#if GAPI_ENABLE_VALIDATION
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DebugUtilsMessengerEXT, Instance)
#endif

struct DeviceLimits
{
   MemorySize min_uniform_buffer_alignment{};
};

namespace vulkan {
using PhysicalDevice = VkPhysicalDevice;
}// namespace vulkan

constexpr auto g_max_mip_maps = 0;

class Device
{
 public:
   Device(vulkan::Device device, vulkan::PhysicalDevice physical_device, std::vector<QueueFamilyInfo>&& queue_family_infos,
          DeviceFeatureFlags enabled_features);

   [[nodiscard]] Result<Swapchain> create_swapchain(const Surface& surface, ColorFormat color_format, ColorSpace color_space,
                                                    const Resolution& resolution, PresentMode present_mode,
                                                    Swapchain* old_swapchain = nullptr);
   [[nodiscard]] Result<Shader> create_shader(PipelineStage stage, std::string_view entrypoint, std::span<const char> code);
   [[nodiscard]] Result<CommandList> create_command_list(WorkTypeFlags flags = WorkType::Graphics) const;
   [[nodiscard]] Result<DescriptorPool> create_descriptor_pool(std::span<const std::pair<DescriptorType, u32>> descriptor_counts,
                                                               u32 max_descriptor_count);
   [[nodiscard]] Result<Buffer> create_buffer(BufferUsageFlags usage, uint64_t size);
   [[nodiscard]] Result<Fence> create_fence() const;
   [[nodiscard]] Result<Semaphore> create_semaphore() const;
   [[nodiscard]] Result<Texture> create_texture_from_ktx(const ktx::Texture& texture, TextureUsageFlags usage_flags,
                                                         TextureState final_state);
   [[nodiscard]] Result<Texture> create_texture(const ColorFormat& format, const Resolution& image_size,
                                                TextureUsageFlags usage_flags = TextureUsage::Sampled | TextureUsage::TransferSrc |
                                                                                TextureUsage::TransferDst,
                                                TextureState initial_texture_state = TextureState::Undefined,
                                                SampleCount sample_count = SampleCount::Single, int mip_count = 1) const;
   [[nodiscard]] Result<Sampler> create_sampler(const SamplerProperties& info);
   [[nodiscard]] Result<QueryPool> create_query_pool(QueryType query_type, u32 timestamp_count);
   [[nodiscard]] Result<ray_tracing::AccelerationStructure>
   create_acceleration_structure(ray_tracing::AccelerationStructureType struct_type, const Buffer& buffer, MemorySize buffer_offset,
                                 MemorySize buffer_size);

   [[nodiscard]] std::pair<Resolution, Resolution> get_surface_resolution_limits(const Surface& surface) const;
   [[nodiscard]] Vector2u get_current_surface_extent(const Surface& surface) const;

   [[nodiscard]] Status submit_command_list(const CommandList& command_list, SemaphoreArrayView wait_semaphores,
                                            SemaphoreArrayView signal_semaphores, const Fence* fence, WorkTypeFlags work_types);
   [[nodiscard]] Status submit_command_list(const CommandList& command_list, const Semaphore& wait_semaphore,
                                            const Semaphore& signal_semaphore, const Fence& fence);
   [[nodiscard]] Status submit_command_list_one_time(const CommandList& command_list);
   [[nodiscard]] VkDevice vulkan_device() const;
   [[nodiscard]] VkPhysicalDevice vulkan_physical_device() const;
   [[nodiscard]] QueueManager& queue_manager();
   [[nodiscard]] SamplerCache& sampler_cache();
   [[nodiscard]] DeviceFeatureFlags enabled_features() const;
   [[nodiscard]] Result<ktx::Texture> export_ktx_texture(const Texture& texture);
   [[nodiscard]] const DeviceLimits& limits() const;

   void await_all() const;

   [[nodiscard]] MemorySize min_storage_buffer_alignment() const;

 private:
   [[nodiscard]] uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

   vulkan::Device m_device;
   vulkan::PhysicalDevice m_physical_device;
   std::vector<QueueFamilyInfo> m_queue_family_infos;
   DeviceFeatureFlags m_enabled_features;
   QueueManager m_queue_manager;
   SamplerCache m_sampler_cache;
};

using DeviceUPtr = std::unique_ptr<Device>;

}// namespace triglav::graphics_api
