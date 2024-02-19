#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include "triglav/desktop/VulkanSurface.h"

namespace triglav::graphics_api::vulkan {

using SurfaceKHR = WrappedObject<VkSurfaceKHR, triglav::desktop::create_vulkan_surface, vkDestroySurfaceKHR, VkInstance>;

}// namespace graphics_api
