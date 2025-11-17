#pragma once

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <vector>

#include "../Buffer.hpp"
#include "AccelerationStructure.hpp"

namespace triglav::graphics_api::ray_tracing {

class InstanceBuilder
{
 public:
   explicit InstanceBuilder(Device& device);

   void add_instance(AccelerationStructure& acc_structure, glm::mat4 matrix, Index instance_index);

   Buffer build_buffer();

 private:
   Device& m_device;
   std::vector<VkAccelerationStructureInstanceKHR> m_instances;
};

}// namespace triglav::graphics_api::ray_tracing
