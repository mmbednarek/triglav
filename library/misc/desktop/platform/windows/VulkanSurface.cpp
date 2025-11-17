#include "VulkanSurface.hpp"

#include "Surface.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace triglav::desktop {

VkResult create_vulkan_surface(const VkInstance instance, const ISurface* surface_iface, const VkAllocationCallbacks* p_allocator,
                               VkSurfaceKHR* p_surface)
{
   auto* surface = dynamic_cast<const Surface*>(surface_iface);

   VkWin32SurfaceCreateInfoKHR surface_info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
   surface_info.hinstance = surface->winapi_instance();
   surface_info.hwnd = surface->winapi_window_handle();

   return vkCreateWin32SurfaceKHR(instance, &surface_info, p_allocator, p_surface);
}

const char* vulkan_extension_name(const IDisplay* /*display*/)
{
   return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

}// namespace triglav::desktop