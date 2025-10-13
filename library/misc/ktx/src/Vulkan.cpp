#include "Vulkan.hpp"

#include "Texture.hpp"

#include <ktxvulkan.h>
#include <utility>

namespace triglav::ktx {

VulkanDeviceInfo::VulkanDeviceInfo(const VkPhysicalDevice physicalDevice, const VkDevice device, const VkQueue queue,
                                   const VkCommandPool cmdPool) :
    m_deviceInfo(ktxVulkanDeviceInfo_Create(physicalDevice, device, queue, cmdPool, nullptr))
{
}

VulkanDeviceInfo::~VulkanDeviceInfo()
{
   if (m_deviceInfo != nullptr) {
      ktxVulkanDeviceInfo_Destroy(m_deviceInfo);
   }
}

VulkanDeviceInfo::VulkanDeviceInfo(VulkanDeviceInfo&& other) noexcept :
    m_deviceInfo(std::exchange(other.m_deviceInfo, nullptr))
{
}

VulkanDeviceInfo& VulkanDeviceInfo::operator=(VulkanDeviceInfo&& other) noexcept
{
   m_deviceInfo = std::exchange(other.m_deviceInfo, nullptr);
   return *this;
}

std::optional<VulkanTexture> VulkanDeviceInfo::upload_texture(const Texture& texture, const VkImageTiling tiling,
                                                              const VkImageUsageFlags usageFlags, const VkImageLayout finalLayout) const
{
   ktxVulkanTexture vulkanTex{};
   auto result = ktxTexture_VkUploadEx(texture.m_ktxTexture, m_deviceInfo, &vulkanTex, tiling, usageFlags, finalLayout);
   if (result != KTX_SUCCESS) {
      return std::nullopt;
   }

   return VulkanTexture{.image = vulkanTex.image,
                        .memory = vulkanTex.deviceMemory,
                        .format = vulkanTex.imageFormat,
                        .usageFlags = usageFlags,
                        .imageSize = {vulkanTex.width, vulkanTex.height},
                        .mipCount = vulkanTex.levelCount};
}

}// namespace triglav::ktx