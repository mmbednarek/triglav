#pragma once

#include "Buffer.h"
#include "GraphicsApi.hpp"
#include "QueueManager.h"
#include "Sampler.h"
#include "SamplerCache.h"
#include "Shader.h"
#include "Surface.hpp"
#include "Swapchain.h"
#include "Synchronization.h"
#include "Texture.h"
#include "TimestampArray.h"
#include "vulkan/ObjectWrapper.hpp"

#include <memory>
#include <numeric>
#include <optional>
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

DECLARE_VLK_WRAPPED_OBJECT(Instance)
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
   Device(vulkan::Instance instance,
#if GAPI_ENABLE_VALIDATION
          vulkan::DebugUtilsMessengerEXT debugMessenger,
#endif
          vulkan::SurfaceKHR surface, vulkan::Device device, vulkan::PhysicalDevice physicalDevice,
          std::vector<QueueFamilyInfo>&& queueFamilyInfos);

   [[nodiscard]] Result<Swapchain> create_swapchain(ColorFormat colorFormat, ColorSpace colorSpace, const Resolution& resolution,
                                                    Swapchain* oldSwapchain = nullptr);
   [[nodiscard]] Result<Shader> create_shader(PipelineStage stage, std::string_view entrypoint, std::span<const char> code);
   [[nodiscard]] Result<CommandList> create_command_list(WorkTypeFlags flags = WorkType::Graphics) const;
   [[nodiscard]] Result<Buffer> create_buffer(BufferUsageFlags usage, uint64_t size);
   [[nodiscard]] Result<Fence> create_fence() const;
   [[nodiscard]] Result<Semaphore> create_semaphore() const;
   [[nodiscard]] Result<Texture> create_texture(const ColorFormat& format, const Resolution& imageSize,
                                                TextureUsageFlags usageFlags = TextureUsage::Sampled | TextureUsage::TransferSrc |
                                                                               TextureUsage::TransferDst,
                                                SampleCount sampleCount = SampleCount::Single, int mipCount = 1) const;
   [[nodiscard]] Result<Sampler> create_sampler(const SamplerProperties& info);
   [[nodiscard]] Result<TimestampArray> create_timestamp_array(u32 timestampCount);

   [[nodiscard]] std::pair<Resolution, Resolution> get_surface_resolution_limits() const;

   [[nodiscard]] Status submit_command_list(const CommandList& commandList, SemaphoreArrayView waitSemaphores,
                                            SemaphoreArrayView signalSemaphores, const Fence* fence, WorkTypeFlags workTypes);
   [[nodiscard]] Status submit_command_list(const CommandList& commandList, const Semaphore& waitSemaphore,
                                            const Semaphore& signalSemaphore, const Fence& fence);
   [[nodiscard]] Status submit_command_list_one_time(const CommandList& commandList);
   [[nodiscard]] VkDevice vulkan_device() const;
   [[nodiscard]] QueueManager& queue_manager();
   [[nodiscard]] SamplerCache& sampler_cache();
   void await_all() const;

 private:
   [[nodiscard]] uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

   vulkan::Instance m_instance;
#if GAPI_ENABLE_VALIDATION
   vulkan::DebugUtilsMessengerEXT m_debugMessenger;
#endif
   vulkan::SurfaceKHR m_surface;
   vulkan::Device m_device;
   vulkan::PhysicalDevice m_physicalDevice;
   std::vector<QueueFamilyInfo> m_queueFamilyInfos;
   QueueManager m_queueManager;
   SamplerCache m_samplerCache;
};

using DeviceUPtr = std::unique_ptr<Device>;

Result<DeviceUPtr> initialize_device(const triglav::desktop::ISurface& surface);

}// namespace triglav::graphics_api
