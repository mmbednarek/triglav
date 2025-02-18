#pragma once

extern "C" {
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
   VkImageUsageFlags usageFlags;
   Vector2u imageSize;
   u32 mipCount;
};

class VulkanDeviceInfo
{
 public:
   explicit VulkanDeviceInfo(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool cmdPool);
   ~VulkanDeviceInfo();

   VulkanDeviceInfo(const VulkanDeviceInfo& other) = delete;
   VulkanDeviceInfo& operator=(const VulkanDeviceInfo& other) = delete;

   VulkanDeviceInfo(VulkanDeviceInfo&& other) noexcept;
   VulkanDeviceInfo& operator=(VulkanDeviceInfo&& other) noexcept;

   [[nodiscard]] std::optional<VulkanTexture> upload_texture(const Texture& texture, VkImageTiling tiling,
                                                                VkImageUsageFlags usageFlags, VkImageLayout finalLayout) const;

 private:
   ::ktxVulkanDeviceInfo* m_deviceInfo;
};

}