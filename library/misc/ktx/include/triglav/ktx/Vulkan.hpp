#pragma once

extern "C"
{
#include "ForwardDecl.h"
#include <vulkan/vulkan.h>
}
#include <optional>

#include "triglav/Math.hpp"

namespace triglav::ktx {

class Texture;

struct VulkanTexture
{
   VkImage image;
   VkDeviceMemory memory;
   VkFormat format;
   VkImageUsageFlags usage_flags;
   Vector2u image_size;
   u32 mip_count;
};

class VulkanDeviceInfo
{
 public:
   explicit VulkanDeviceInfo(VkPhysicalDevice physical_device, VkDevice device, VkQueue queue, VkCommandPool cmd_pool);
   ~VulkanDeviceInfo();

   VulkanDeviceInfo(const VulkanDeviceInfo& other) = delete;
   VulkanDeviceInfo& operator=(const VulkanDeviceInfo& other) = delete;

   VulkanDeviceInfo(VulkanDeviceInfo&& other) noexcept;
   VulkanDeviceInfo& operator=(VulkanDeviceInfo&& other) noexcept;

   [[nodiscard]] std::optional<VulkanTexture> upload_texture(const Texture& texture, VkImageTiling tiling, VkImageUsageFlags usage_flags,
                                                             VkImageLayout final_layout) const;

 private:
   ::ktxVulkanDeviceInfo* m_device_info;
};

}// namespace triglav::ktx