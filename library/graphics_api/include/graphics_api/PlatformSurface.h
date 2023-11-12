#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#include <Windows.h>
#elif __unix__
#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>
#endif

namespace graphics_api {

struct Surface
{
#if GAPI_PLATFORM_WINDOWS
   HWND windowHandle;
   HINSTANCE processInstance;
#elif GAPI_PLATFORM_WAYLAND
   wl_display *display;
   wl_surface *surface;
#endif
};

namespace vulkan {
#if GAPI_PLATFORM_WINDOWS
using SurfaceKHR = WrappedObject<VkSurfaceKHR, vkCreateWin32SurfaceKHR, vkDestroySurfaceKHR, VkInstance>;
#elif GAPI_PLATFORM_WAYLAND
using SurfaceKHR = WrappedObject<VkSurfaceKHR, vkCreateWaylandSurfaceKHR, vkDestroySurfaceKHR, VkInstance>;
#endif

Status to_vulkan_surface(SurfaceKHR &outVulkanSurface, const Surface &surface);
}// namespace vulkan

}// namespace graphics_api
