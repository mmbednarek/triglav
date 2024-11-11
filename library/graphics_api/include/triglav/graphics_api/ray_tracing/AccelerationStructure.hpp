#pragma once

#include "../vulkan/DynamicProcedures.hpp"
#include "../vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {
DECLARE_VLK_WRAPPED_CHILD_OBJECT(AccelerationStructureKHR, Device);
}
namespace triglav::graphics_api::ray_tracing {

class AccelerationStructure
{
 public:
   explicit AccelerationStructure(vulkan::AccelerationStructureKHR structure);

   [[nodiscard]] const VkAccelerationStructureKHR& vulkan_acceleration_structure() const;
   VkDeviceAddress vulkan_device_address();

 private:
   vulkan::AccelerationStructureKHR m_structure;
};

}// namespace triglav::graphics_api::ray_tracing
