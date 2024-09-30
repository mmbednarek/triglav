#include "ray_tracing/AccelerationStructurePool.hpp"
#include "Buffer.hpp"
#include "Device.hpp"

#include "triglav/desktop/ISurfaceEventListener.hpp"

#include <ranges>
#include <spdlog/spdlog.h>

namespace triglav::graphics_api::ray_tracing {

constexpr auto CHUNK_SIZE = 1024ull;
constexpr auto CHUNK_COUNT = 2048ull;
constexpr auto PAGE_SIZE = CHUNK_SIZE * CHUNK_COUNT;

using ChunkCount = Index;

namespace {

ChunkCount memory_size_to_chunk_count(const MemorySize size)
{
   return size % CHUNK_SIZE == 0 ? size / CHUNK_SIZE : size / CHUNK_SIZE + 1;
}

}// namespace

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

   auto section = m_accStructHeap.allocate_section(size);
   auto as = GAPI_CHECK(m_device.create_acceleration_structure(type, *section.buffer, section.offset, section.size));
   auto* result = new AccelerationStructure(std::move(as));
   m_sections.emplace(result, section);
   return result;
}

void AccelerationStructurePool::release_acceleration_structure(const AccelerationStructure* as)
{
   spdlog::trace("as-pool: Releasing acceleration structure");

   const auto& section = m_sections.at(as);
   m_accStructHeap.release_section(section);
   delete as;
}

}// namespace triglav::graphics_api::ray_tracing
