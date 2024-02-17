#pragma once

#include <vulkan/vulkan.h>

namespace triglav::desktop {

class ISurface;

VkResult create_vulkan_surface(VkInstance instance, const ISurface *surfaceIFace,
                               const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);
const char* vulkan_extension_name();

}// namespace triglav::desktop