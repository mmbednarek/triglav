#pragma once

#include "../Device.hpp"
#include "../vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api::ray_tracing {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(AccelerationStructureKHR, Device);

class AccelerationStructure
{
 public:
   explicit AccelerationStructure(vulkan::AccelerationStructureKHR structure);

   vulkan::AccelerationStructureKHR& vulkan_acceleration_structure();

 private:
   vulkan::AccelerationStructureKHR m_structure;
};

}// namespace triglav::graphics_api::ray_tracing
