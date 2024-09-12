#include "ray_tracing/AccelerationStructure.hpp"
#include "vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api::ray_tracing {

AccelerationStructure::AccelerationStructure(vulkan::AccelerationStructureKHR structure) :
   m_structure(std::move(structure))
{
}

vulkan::AccelerationStructureKHR& AccelerationStructure::vulkan_acceleration_structure()
{
   return m_structure;
}

VkDeviceAddress AccelerationStructure::vulkan_device_address()
{
   VkAccelerationStructureDeviceAddressInfoKHR info{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
   info.accelerationStructure = *m_structure;
   return vulkan::vkGetAccelerationStructureDeviceAddressKHR(m_structure.parent(), &info);
}

}
