#include "VulkanSurface.hpp"

#include "Display.hpp"

#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>

#include "Surface.hpp"

namespace triglav::desktop {

VkResult create_vulkan_surface(const VkInstance instance, const ISurface* surfaceIFace, const VkAllocationCallbacks* pAllocator,
                               VkSurfaceKHR* pSurface)
{
   auto* surface = dynamic_cast<const x11::Surface*>(surfaceIFace);

   VkXlibSurfaceCreateInfoKHR info{};
   info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
   info.pNext = nullptr;
   info.flags = 0;
   info.dpy = surface->display().x11_display();
   info.window = surface->window();

   return vkCreateXlibSurfaceKHR(instance, &info, pAllocator, pSurface);
}

const char* vulkan_extension_name()
{
   return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

}// namespace triglav::desktop
