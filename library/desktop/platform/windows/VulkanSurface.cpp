#include "VulkanSurface.hpp"

#include "Surface.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace triglav::desktop {

VkResult create_vulkan_surface(const VkInstance instance, const ISurface* surfaceIFace, const VkAllocationCallbacks* pAllocator,
                               VkSurfaceKHR* pSurface)
{
   auto* surface = dynamic_cast<const Surface*>(surfaceIFace);

   VkWin32SurfaceCreateInfoKHR surfaceInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
   surfaceInfo.hinstance = surface->winapi_instance();
   surfaceInfo.hwnd = surface->winapi_window_handle();

   return vkCreateWin32SurfaceKHR(instance, &surfaceInfo, pAllocator, pSurface);
}

const char* vulkan_extension_name(const IDisplay* /*display*/)
{
   return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

}// namespace triglav::desktop