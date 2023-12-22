#pragma once

#include "Buffer.h"
#include "GraphicsApi.hpp"
#include "Pipeline.h"
#include "PlatformSurface.h"
#include "RenderPass.h"
#include "Sampler.h"
#include "Shader.h"
#include "Swapchain.h"
#include "Synchronization.h"
#include "Texture.h"
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

namespace graphics_api {

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
   uint32_t presentQueue;
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

class Device
{
 public:
   Device(vulkan::Instance instance,
#if GAPI_ENABLE_VALIDATION
          vulkan::DebugUtilsMessengerEXT debugMessenger,
#endif
          vulkan::SurfaceKHR surface, vulkan::Device device, vulkan::PhysicalDevice physicalDevice,
          const vulkan::QueueFamilyIndices &queueFamilies, vulkan::CommandPool commandPool);

   [[nodiscard]] Result<Swapchain> create_swapchain(ColorFormat colorFormat,
                                                    ColorSpace colorSpace, ColorFormat depthFormat,
                                                    SampleCount sampleCount, const Resolution &resolution,
                                                    Swapchain *oldSwapchain = nullptr);
   [[nodiscard]] Result<RenderPass> create_render_pass(IRenderTarget& renderTarget);
   [[nodiscard]] Result<Pipeline> create_pipeline(const RenderPass &renderPass,
                                                  std::span<const Shader *> shaders,
                                                  std::span<VertexInputLayout> layouts,
                                                  std::span<DescriptorBinding> descriptorBindings,
                                                  bool enableDepthTest);
   [[nodiscard]] Result<Shader> create_shader(ShaderStage stage, std::string_view entrypoint,
                                              std::span<const uint8_t> code);
   [[nodiscard]] Result<CommandList> create_command_list() const;
   [[nodiscard]] Result<Buffer> create_buffer(BufferPurpose purpose, uint64_t size);
   [[nodiscard]] Result<Fence> create_fence() const;
   [[nodiscard]] Result<Semaphore> create_semaphore() const;
   [[nodiscard]] Result<Texture> create_texture(const ColorFormat &format, const Resolution &imageSize,
                                                TextureType type        = TextureType::SampledImage,
                                                SampleCount sampleCount = SampleCount::Bits1) const;
   [[nodiscard]] Result<Sampler> create_sampler(bool enableAnisotropy);

   [[nodiscard]] std::pair<Resolution, Resolution> get_surface_resolution_limits() const;

   [[nodiscard]] Status submit_command_list(const CommandList &commandList, const Semaphore &waitSemaphore,
                                            const Semaphore &signalSemaphore, const Fence &fence) const;
   [[nodiscard]] Status submit_command_list_one_time(const CommandList &commandList) const;
   [[nodiscard]] VkDevice vulkan_device() const;
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
   vulkan::QueueFamilyIndices m_queueFamilies;
   vulkan::CommandPool m_commandPool;
   VkQueue m_graphicsQueue;
};

using DeviceUPtr = std::unique_ptr<Device>;

Result<DeviceUPtr> initialize_device(const Surface &surface);

}// namespace graphics_api
