#include "wayland/Display.hpp"
#include "wayland/Surface.hpp"
#include "x11/Display.hpp"
#include "x11/Surface.hpp"

#include "triglav/desktop/VulkanSurface.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xlib.h>

namespace triglav::desktop {

VkResult create_vulkan_surface(const VkInstance instance, const ISurface* surfaceIFace, const VkAllocationCallbacks* pAllocator,
                               VkSurfaceKHR* pSurface)
{
   auto* waylandSurface = dynamic_cast<const wayland::Surface*>(surfaceIFace);
   if (waylandSurface != nullptr) {
      VkWaylandSurfaceCreateInfoKHR info{};
      info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
      info.pNext = nullptr;
      info.flags = 0;
      info.display = waylandSurface->display().display();
      info.surface = waylandSurface->surface();
      return vkCreateWaylandSurfaceKHR(instance, &info, pAllocator, pSurface);
   }

   auto* x11Surface = dynamic_cast<const x11::Surface*>(surfaceIFace);
   VkXlibSurfaceCreateInfoKHR info{};
   info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
   info.pNext = nullptr;
   info.flags = 0;
   info.dpy = x11Surface->display().x11_display();
   info.window = x11Surface->window();
   return vkCreateXlibSurfaceKHR(instance, &info, pAllocator, pSurface);
}

const char* vulkan_extension_name(const IDisplay* display)
{
   auto* waylandDisplay = dynamic_cast<const wayland::Display*>(display);
   return (waylandDisplay != nullptr) ? VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME : VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

}// namespace triglav::desktop
