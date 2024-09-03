#include "ray_tracing/AccelerationStructure.hpp"

namespace triglav::graphics_api::ray_tracing {

AccelerationStructure::AccelerationStructure(vulkan::AccelerationStructureKHR structure) :
   m_structure(std::move(structure))
{
}

vulkan::AccelerationStructureKHR& AccelerationStructure::vulkan_acceleration_structure()
{
   return m_structure;
}

}
