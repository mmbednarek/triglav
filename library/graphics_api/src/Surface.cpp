#include "Surface.hpp"

#include <utility>

namespace triglav::graphics_api {

Surface::Surface(vulkan::SurfaceKHR&& surface) :
    m_surface(std::move(surface))
{
}

VkSurfaceKHR Surface::vulkan_surface() const
{
   return *m_surface;
}

}// namespace triglav::graphics_api
