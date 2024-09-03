#pragma once

#include "../vulkan/ObjectWrapper.hpp"
#include "../vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api {
DECLARE_VLK_WRAPPED_CHILD_OBJECT(AccelerationStructureKHR, Device);
}
namespace triglav::graphics_api::ray_tracing {

class AccelerationStructure
{
 public:
   explicit AccelerationStructure(vulkan::AccelerationStructureKHR structure);

   vulkan::AccelerationStructureKHR& vulkan_acceleration_structure();

 private:
   vulkan::AccelerationStructureKHR m_structure;
};

}// namespace triglav::graphics_api::ray_tracing
