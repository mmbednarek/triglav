#include "ray_tracing/AccelerationStructurePool.hpp"
#include "Buffer.hpp"
#include "Device.hpp"

namespace triglav::graphics_api::ray_tracing {

AccelerationStructurePool::AccelerationStructurePool(Device& device) :
   m_device(device)
{
}

AccelerationStructure* AccelerationStructurePool::acquire_acceleration_structure(AccelerationStructureType type, MemorySize size)
{
   // TODO: Reuse created acceleration structures
   auto buffer = GAPI_CHECK(m_device.create_buffer(BufferUsage::AccelerationStructure, size));
   auto as = GAPI_CHECK(m_device.create_acceleration_structure(type, buffer));
   auto result = new AccelerationStructure(std::move(as));
   m_asToBufferMap.emplace(result, new Buffer(std::move(buffer)));
   return result;
}

void AccelerationStructurePool::release_acceleration_structure(AccelerationStructure* as)
{
   // TODO: Reuse created acceleration structures
   auto* buffer = m_asToBufferMap[as];
   m_asToBufferMap.erase(as);
   delete as;
   delete buffer;
}

}
