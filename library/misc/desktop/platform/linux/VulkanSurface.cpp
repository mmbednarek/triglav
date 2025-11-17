#include "wayland/Display.hpp"
#include "wayland/Surface.hpp"
#include "x11/Display.hpp"
#include "x11/Surface.hpp"

#include "triglav/desktop/VulkanSurface.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xlib.h>

namespace triglav::desktop {

VkResult create_vulkan_surface(const VkInstance instance, const ISurface* surface_iface, const VkAllocationCallbacks* p_allocator,
                               VkSurfaceKHR* p_surface)
{
   auto* wayland_surface = dynamic_cast<const wayland::Surface*>(surface_iface);
   if (wayland_surface != nullptr) {
      VkWaylandSurfaceCreateInfoKHR info{};
      info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
      info.pNext = nullptr;
      info.flags = 0;
      info.display = wayland_surface->display().display();
      info.surface = wayland_surface->surface();
      return vkCreateWaylandSurfaceKHR(instance, &info, p_allocator, p_surface);
   }

   auto* x11_surface = dynamic_cast<const x11::Surface*>(surface_iface);
   VkXlibSurfaceCreateInfoKHR info{};
   info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
   info.pNext = nullptr;
   info.flags = 0;
   info.dpy = x11_surface->display().x11_display();
   info.window = x11_surface->window();
   return vkCreateXlibSurfaceKHR(instance, &info, p_allocator, p_surface);
}

const char* vulkan_extension_name(const IDisplay* display)
{
   auto* wayland_display = dynamic_cast<const wayland::Display*>(display);
   return (wayland_display != nullptr) ? VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME : VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

}// namespace triglav::desktop
