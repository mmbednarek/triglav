#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <glm/mat4x4.hpp>

#include "AccelerationStructure.hpp"
#include "../Buffer.hpp"

namespace triglav::graphics_api::ray_tracing {

class InstanceBuilder {
 public:
   explicit InstanceBuilder(Device& device);

   void add_instance(AccelerationStructure& accStructure, glm::mat4 matrix);

   Buffer build_buffer();

 private:
   Device &m_device;
   std::vector<VkAccelerationStructureInstanceKHR> m_instances;

};

}
