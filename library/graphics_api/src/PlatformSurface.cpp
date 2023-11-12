#include "PlatformSurface.h"

namespace graphics_api::vulkan {

Status to_vulkan_surface(SurfaceKHR &outVulkanSurface, const Surface &surface)
{
#if GAPI_PLATFORM_WINDOWS
   VkWin32SurfaceCreateInfoKHR createInfo{};
   createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
   createInfo.hwnd      = surface.windowHandle;
   createInfo.hinstance = surface.processInstance;

   if (const auto res = outVulkanSurface.construct(&createInfo); res != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
#elif GAPI_PLATFORM_WAYLAND
   VkWaylandSurfaceCreateInfoKHR createInfo{};
   createInfo.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
   createInfo.display = surface.display;
   createInfo.surface = surface.surface;

   if (const auto res = outVulkanSurface.construct(&createInfo); res != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
#endif
}

}// namespace graphics_api::vulkan
