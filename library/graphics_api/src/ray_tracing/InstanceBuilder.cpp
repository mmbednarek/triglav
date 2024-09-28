#include "ray_tracing/InstanceBuilder.hpp"

#include <cstring>

#include "Device.hpp"

namespace triglav::graphics_api::ray_tracing {

InstanceBuilder::InstanceBuilder(Device& device) :
    m_device(device)
{
}

void InstanceBuilder::add_instance(AccelerationStructure& accStructure, glm::mat4 matrix, const Index instanceIndex)
{
   VkAccelerationStructureInstanceKHR instance{};
   instance.accelerationStructureReference = accStructure.vulkan_device_address();
   instance.instanceCustomIndex = instanceIndex;
   instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
   instance.mask = 0xFF;
   instance.instanceShaderBindingTableRecordOffset = 0;

   auto matT = glm::transpose(matrix);
   std::memcpy(instance.transform.matrix, &matT, sizeof(VkTransformMatrixKHR));

   m_instances.emplace_back(instance);
}

Buffer InstanceBuilder::build_buffer()
{
   auto buffer = GAPI_CHECK(m_device.create_buffer(BufferUsage::AccelerationStructureRead | BufferUsage::TransferDst,
                                                   m_instances.size() * sizeof(VkAccelerationStructureInstanceKHR)));
   GAPI_CHECK_STATUS(buffer.write_indirect(m_instances.data(), m_instances.size() * sizeof(VkAccelerationStructureInstanceKHR)));
   return buffer;
}

}// namespace triglav::graphics_api::ray_tracing
