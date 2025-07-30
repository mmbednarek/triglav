#pragma once

#include <vulkan/vulkan.h>

namespace triglav::desktop {

class ISurface;
class IDisplay;

VkResult create_vulkan_surface(VkInstance instance, const ISurface* surfaceIFace, const VkAllocationCallbacks* pAllocator,
                               VkSurfaceKHR* pSurface);
const char* vulkan_extension_name(const IDisplay* display);

}// namespace triglav::desktop