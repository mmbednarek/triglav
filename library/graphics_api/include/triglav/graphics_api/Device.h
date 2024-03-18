#pragma once

#include "Buffer.h"
#include "DepthRenderTarget.h"
#include "GraphicsApi.hpp"
#include "Surface.hpp"
#include "RenderPass.h"
#include "Sampler.h"
#include "Shader.h"
#include "Swapchain.h"
#include "Synchronization.h"
#include "Texture.h"
#include "TextureRenderTarget.h"
#include "QueueManager.h"
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
DECLARE_VLK_WRAPPED_CHILD_OBJECT(CommandPool, Device)

#if GAPI_ENABLE_VALIDATION
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DebugUtilsMessengerEXT, Instance)
#endif

constexpr uint32_t g_invalidQueueIndex = std::numeric_limits<uint32_t>::max();

namespace vulkan {
using PhysicalDevice = VkPhysicalDevice;

struct QueueFamilyIndices
{
   uint32_t graphicsQueue;
   uint32_t graphicsQueueCount;
   uint32_t presentQueue;
   uint32_t presentQueueCount;
};
}// namespace vulkan

struct RenderPassCreateInfo
{
   Resolution resolution;
   bool is_depth_enabled;
   ColorFormat depth_format;
   SampleCount sample_count;
   std::vector<ColorFormat> color_image_formats;
};

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

   [[nodiscard]] Result<Swapchain> create_swapchain(ColorFormat colorFormat, ColorSpace colorSpace,
                                                    ColorFormat depthFormat, SampleCount sampleCount,
                                                    const Resolution &resolution,
                                                    Swapchain *oldSwapchain = nullptr);
   [[nodiscard]] Result<RenderPass> create_render_pass(IRenderTarget &renderTarget);
   [[nodiscard]] Result<Shader> create_shader(PipelineStage stage, std::string_view entrypoint,
                                              std::span<const char> code);
   [[nodiscard]] Result<CommandList> create_command_list(WorkTypeFlags flags = WorkType::Graphics) const;
   [[nodiscard]] Result<Buffer> create_buffer(BufferPurpose purpose, uint64_t size);
   [[nodiscard]] Result<Fence> create_fence() const;
   [[nodiscard]] Result<Semaphore> create_semaphore() const;
   [[nodiscard]] Result<Texture> create_texture(const ColorFormat &format, const Resolution &imageSize,
                                                TextureType type        = TextureType::SampledImage,
                                                SampleCount sampleCount = SampleCount::Single,
                                                int mipCount = 1) const;
   [[nodiscard]] Result<Sampler> create_sampler(const SamplerInfo& info);
   [[nodiscard]] Result<DepthRenderTarget> create_depth_render_target(const ColorFormat &depthFormat,
                                                                      const Resolution &resolution) const;
   [[nodiscard]] Result<TextureRenderTarget> create_texture_render_target(const Resolution &resolution) const;

   [[nodiscard]] std::pair<Resolution, Resolution> get_surface_resolution_limits() const;

   [[nodiscard]] Status submit_command_list(const CommandList &commandList, const SemaphoreArray &waitSemaphores,
                                            const Semaphore &signalSemaphore, const Fence *fence) const;
   [[nodiscard]] Status submit_command_list(const CommandList &commandList, const Semaphore &waitSemaphore,
                                            const Semaphore &signalSemaphore, const Fence &fence) const;
   [[nodiscard]] Status submit_command_list_one_time(const CommandList &commandList) const;
   [[nodiscard]] VkDevice vulkan_device() const;
   [[nodiscard]] QueueManager& queue_manager();
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
};

using DeviceUPtr = std::unique_ptr<Device>;

Result<DeviceUPtr> initialize_device(const triglav::desktop::ISurface &surface);

}// namespace graphics_api
