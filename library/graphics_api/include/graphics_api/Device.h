#pragma once

#include "Buffer.h"
#include "CommandList.h"
#include "GraphicsApi.hpp"
#include "Pipeline.h"
#include "PlatformSurface.h"
#include "Shader.h"
#include "Synchronization.h"
#include "Texture.h"
#include "vulkan/ObjectWrapper.hpp"

#include <memory>
#include <span>
#include <vector>

#if NDEBUG
#define GAPI_ENABLE_VALIDATION 0
#else
#define GAPI_ENABLE_VALIDATION 1
#endif

namespace graphics_api {

DECLARE_VLK_WRAPPED_OBJECT(Instance)
DECLARE_VLK_WRAPPED_OBJECT(Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(SwapchainKHR, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(RenderPass, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(Framebuffer, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(CommandPool, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(Sampler, Device)

#if GAPI_ENABLE_VALIDATION
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DebugUtilsMessengerEXT, Instance)
#endif

namespace vulkan {
using PhysicalDevice = VkPhysicalDevice;

struct QueueFamilyIndices
{
   uint32_t graphicsQueue;
   uint32_t presentQueue;
};
}// namespace vulkan

class Device
{
 public:
   Device(vulkan::Instance instance,
#if GAPI_ENABLE_VALIDATION
          vulkan::DebugUtilsMessengerEXT debugMessenger,
#endif
          vulkan::SurfaceKHR surface, vulkan::Device device, vulkan::PhysicalDevice physicalDevice,
          const vulkan::QueueFamilyIndices &queueFamilies, vulkan::CommandPool commandPool);

   [[nodiscard]] Status init_color_format(const ColorFormat &colorFormat, ColorSpace colorSpace);
   [[nodiscard]] Status init_swapchain(const Resolution &resolution);

   [[nodiscard]] bool is_surface_format_supported(const ColorFormat &colorFormat,
                                                  ColorSpace colorSpace) const;
   [[nodiscard]] std::pair<Resolution, Resolution> get_surface_resolution_limits() const;

   [[nodiscard]] Result<Pipeline> create_pipeline(std::span<Shader *> shaders,
                                                  std::span<VertexInputLayout> layouts,
                                                  std::span<DescriptorBinding> descriptorBindings);
   [[nodiscard]] Result<Shader> create_shader(ShaderStage stage, std::string_view entrypoint,
                                              std::span<const uint8_t> code);
   [[nodiscard]] Result<CommandList> create_command_list() const;
   [[nodiscard]] Result<Buffer> create_buffer(BufferPurpose purpose, uint64_t size);
   [[nodiscard]] Result<Fence> create_fence() const;
   [[nodiscard]] Result<Semaphore> create_semaphore() const;
   [[nodiscard]] Result<Texture> create_texture(ColorFormat format, const Resolution &imageSize) const;

   [[nodiscard]] Status begin_graphic_commands(CommandList &commandList, uint32_t framebufferIndex,
                                               const Color &clearColor) const;
   [[nodiscard]] Status begin_one_time_commands(CommandList &commandList) const;

   [[nodiscard]] uint32_t get_available_framebuffer(const Semaphore &semaphore) const;
   [[nodiscard]] Status submit_command_list(const CommandList &commandList, const Semaphore &waitSemaphore,
                                            const Semaphore &signalSemaphore, const Fence &fence) const;
   [[nodiscard]] Status submit_command_list_one_time(const CommandList &commandList) const;
   [[nodiscard]] Status present(const Semaphore &semaphore, uint32_t framebufferIndex) const;
   [[nodiscard]] uint32_t framebuffer_count() const;
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
   vulkan::SwapchainKHR m_swapchain;
   Resolution m_surfaceResolution;
   ColorFormat m_surfaceFormat;
   ColorSpace m_colorSpace;
   vulkan::QueueFamilyIndices m_queueFamilies;
   std::vector<vulkan::ImageView> m_swapchainImageViews;
   std::vector<vulkan::Framebuffer> m_swapchainFramebuffers;
   vulkan::RenderPass m_renderPass;
   vulkan::CommandPool m_commandPool;
   vulkan::Sampler m_sampler;
   VkQueue m_graphicsQueue;
   VkQueue m_presentQueue;
};

using DeviceUPtr = std::unique_ptr<Device>;

Result<DeviceUPtr> initialize_device(const Surface &surface);

}// namespace graphics_api
