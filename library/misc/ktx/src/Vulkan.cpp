#include "Vulkan.hpp"

#include "Texture.hpp"

#include <ktxvulkan.h>
#include <utility>

namespace triglav::ktx {

VulkanDeviceInfo::VulkanDeviceInfo(const VkPhysicalDevice physical_device, const VkDevice device, const VkQueue queue,
                                   const VkCommandPool cmd_pool) :
    m_device_info(ktxVulkanDeviceInfo_Create(physical_device, device, queue, cmd_pool, nullptr))
{
}

VulkanDeviceInfo::~VulkanDeviceInfo()
{
   if (m_device_info != nullptr) {
      ktxVulkanDeviceInfo_Destroy(m_device_info);
   }
}

VulkanDeviceInfo::VulkanDeviceInfo(VulkanDeviceInfo&& other) noexcept :
    m_device_info(std::exchange(other.m_device_info, nullptr))
{
}

VulkanDeviceInfo& VulkanDeviceInfo::operator=(VulkanDeviceInfo&& other) noexcept
{
   m_device_info = std::exchange(other.m_device_info, nullptr);
   return *this;
}

std::optional<VulkanTexture> VulkanDeviceInfo::upload_texture(const Texture& texture, const VkImageTiling tiling,
                                                              const VkImageUsageFlags usage_flags, const VkImageLayout final_layout) const
{
   ktxVulkanTexture vulkan_tex{};
   auto result = ktxTexture_VkUploadEx(texture.m_ktxTexture, m_device_info, &vulkan_tex, tiling, usage_flags, final_layout);
   if (result != KTX_SUCCESS) {
      return std::nullopt;
   }

   return VulkanTexture{.image = vulkan_tex.image,
                        .memory = vulkan_tex.deviceMemory,
                        .format = vulkan_tex.imageFormat,
                        .usage_flags = usage_flags,
                        .image_size = {vulkan_tex.width, vulkan_tex.height},
                        .mip_count = vulkan_tex.levelCount};
}

}// namespace triglav::ktx