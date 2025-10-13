#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include "triglav/desktop/VulkanSurface.hpp"

namespace triglav::graphics_api {

namespace vulkan {
using SurfaceKHR = WrappedObject<VkSurfaceKHR, desktop::create_vulkan_surface, vkDestroySurfaceKHR, VkInstance>;
}

class Surface
{
 public:
   explicit Surface(vulkan::SurfaceKHR&& surface);

   [[nodiscard]] std::pair<Resolution, Resolution> get_limits() const;

   [[nodiscard]] VkSurfaceKHR vulkan_surface() const;

 private:
   vulkan::SurfaceKHR m_surface;
};

}// namespace triglav::graphics_api
