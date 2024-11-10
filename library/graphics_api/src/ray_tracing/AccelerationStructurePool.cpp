#include "ray_tracing/AccelerationStructurePool.hpp"

#include "Buffer.hpp"
#include "Device.hpp"

#include "triglav/desktop/ISurfaceEventListener.hpp"

#include <ranges>
#include <spdlog/spdlog.h>

namespace triglav::graphics_api::ray_tracing {

AccelerationStructurePool::AccelerationStructurePool(Device& device) :
    m_device(device),
    m_accStructHeap(device, BufferUsage::AccelerationStructure)
{
}

AccelerationStructurePool::~AccelerationStructurePool()
{
   for (const auto& as : m_sections | std::views::keys) {
      delete as;
   }
}

AccelerationStructure* AccelerationStructurePool::acquire_acceleration_structure(const AccelerationStructureType type, MemorySize size)
{
   spdlog::trace("as-pool: Acquiring {} acceleration structure, requested size {}",
                 type == AccelerationStructureType::TopLevel ? "top level" : "bottom level", size);

   AccelerationStructure* result{};
   auto it = m_freeAccelerationStructures.upper_bound(size);
   if (it == m_freeAccelerationStructures.end()) {
      auto section = m_accStructHeap.allocate_section(size);
      auto as = GAPI_CHECK(m_device.create_acceleration_structure(type, *section.buffer, section.offset, section.size));
      result = new AccelerationStructure(std::move(as));
      m_sections.emplace(result, section);
   } else {
      result = it->second;
      m_freeAccelerationStructures.erase(it);
   }

   return result;
}

void AccelerationStructurePool::release_acceleration_structure(AccelerationStructure* as)
{
   spdlog::trace("as-pool: Releasing acceleration structure");
   m_freeAccelerationStructures.emplace(m_sections.at(as).size, as);
}

}// namespace triglav::graphics_api::ray_tracing
