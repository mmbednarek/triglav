#pragma once

#include "Buffer.hpp"
#include "GraphicsApi.hpp"
#include "QueueManager.hpp"
#include "Sampler.hpp"
#include "SamplerCache.hpp"
#include "Shader.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "Synchronization.hpp"
#include "Texture.hpp"
#include "TimestampArray.hpp"
#include "ray_tracing/AccelerationStructure.hpp"
#include "ray_tracing/RayTracing.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <memory>
#include <span>
#include <vector>

#if NDEBUG
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

namespace vulkan {
using PhysicalDevice = VkPhysicalDevice;
}// namespace vulkan

constexpr auto g_maxMipMaps = 0;

class Device
{
 public:
   Device(vulkan::Device device, vulkan::PhysicalDevice physicalDevice, std::vector<QueueFamilyInfo>&& queueFamilyInfos,
          DeviceFeatureFlags enabledFeatures);

   [[nodiscard]] Result<Swapchain> create_swapchain(const Surface& surface, ColorFormat colorFormat, ColorSpace colorSpace,
                                                    const Resolution& resolution, PresentMode presentMode,
                                                    Swapchain* oldSwapchain = nullptr);
   [[nodiscard]] Result<Shader> create_shader(PipelineStage stage, std::string_view entrypoint, std::span<const char> code);
   [[nodiscard]] Result<CommandList> create_command_list(WorkTypeFlags flags = WorkType::Graphics) const;
   [[nodiscard]] Result<DescriptorPool> create_descriptor_pool(std::span<const std::pair<DescriptorType, u32>> descriptorCounts,
                                                               u32 maxDescriptorCount);
   [[nodiscard]] Result<Buffer> create_buffer(BufferUsageFlags usage, uint64_t size);
   [[nodiscard]] Result<Fence> create_fence() const;
   [[nodiscard]] Result<Semaphore> create_semaphore() const;
   [[nodiscard]] Result<Texture> create_texture(const ColorFormat& format, const Resolution& imageSize,
                                                TextureUsageFlags usageFlags = TextureUsage::Sampled | TextureUsage::TransferSrc |
                                                                               TextureUsage::TransferDst,
                                                TextureState initialTextureState = TextureState::Undefined,
                                                SampleCount sampleCount = SampleCount::Single, int mipCount = 1) const;
   [[nodiscard]] Result<Sampler> create_sampler(const SamplerProperties& info);
   [[nodiscard]] Result<TimestampArray> create_timestamp_array(u32 timestampCount);
   [[nodiscard]] Result<ray_tracing::AccelerationStructure> create_acceleration_structure(ray_tracing::AccelerationStructureType structType,
                                                                                          const Buffer& buffer, MemorySize bufferOffset,
                                                                                          MemorySize bufferSize);

   [[nodiscard]] std::pair<Resolution, Resolution> get_surface_resolution_limits(const Surface& surface) const;

   [[nodiscard]] Status submit_command_list(const CommandList& commandList, SemaphoreArrayView waitSemaphores,
                                            SemaphoreArrayView signalSemaphores, const Fence* fence, WorkTypeFlags workTypes);
   [[nodiscard]] Status submit_command_list(const CommandList& commandList, const Semaphore& waitSemaphore,
                                            const Semaphore& signalSemaphore, const Fence& fence);
   [[nodiscard]] Status submit_command_list_one_time(const CommandList& commandList);
   [[nodiscard]] VkDevice vulkan_device() const;
   [[nodiscard]] VkPhysicalDevice vulkan_physical_device() const;
   [[nodiscard]] QueueManager& queue_manager();
   [[nodiscard]] SamplerCache& sampler_cache();
   [[nodiscard]] DeviceFeatureFlags enabled_features() const;

   void await_all() const;

   [[nodiscard]] u32 min_storage_buffer_alignment() const;

 private:
   [[nodiscard]] uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

   vulkan::Device m_device;
   vulkan::PhysicalDevice m_physicalDevice;
   std::vector<QueueFamilyInfo> m_queueFamilyInfos;
   DeviceFeatureFlags m_enabledFeatures;
   QueueManager m_queueManager;
   SamplerCache m_samplerCache;
};

using DeviceUPtr = std::unique_ptr<Device>;

}// namespace triglav::graphics_api
