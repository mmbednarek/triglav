#include "VulkanSurface.hpp"

#include <vulkan/vulkan_wayland.h>

#include "Surface.hpp"

namespace triglav::desktop {

VkResult create_vulkan_surface(const VkInstance instance, const ISurface* surfaceIFace, const VkAllocationCallbacks* pAllocator,
                               VkSurfaceKHR* pSurface)
{
   auto* surface = dynamic_cast<const Surface*>(surfaceIFace);

   VkWaylandSurfaceCreateInfoKHR info{};
   info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
   info.pNext = nullptr;
   info.flags = 0;
   info.display = surface->display().display();
   info.surface = surface->surface();

   return vkCreateWaylandSurfaceKHR(instance, &info, pAllocator, pSurface);
}

const char* vulkan_extension_name()
{
   return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
}

}// namespace triglav::desktop