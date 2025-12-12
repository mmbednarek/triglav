#pragma once

#include "../BufferHeap.hpp"
#include "RayTracing.hpp"

#include "triglav/Int.hpp"
#include "triglav/Logging.hpp"
#include "triglav/ObjectPool.hpp"

#include <map>
#include <memory>

namespace triglav::graphics_api {
class Device;
class Buffer;
}// namespace triglav::graphics_api

namespace triglav::graphics_api::ray_tracing {

class AccelerationStructure;

class AccelerationStructurePool
{
   TG_DEFINE_LOG_CATEGORY(AccelerationStructurePool)
 public:
   struct Node
   {
      Index chunk_count;
      bool is_free;
      Node* next;
   };

   struct Page
   {
      std::unique_ptr<Buffer> buffer;
      Index available_chunks;
      Node* head_node;
   };

   explicit AccelerationStructurePool(Device& device);
   ~AccelerationStructurePool();

   AccelerationStructurePool(const AccelerationStructurePool& other) = delete;
   AccelerationStructurePool(AccelerationStructurePool&& other) noexcept = delete;
   AccelerationStructurePool& operator=(const AccelerationStructurePool& other) = delete;
   AccelerationStructurePool& operator=(AccelerationStructurePool&& other) noexcept = delete;

   AccelerationStructure* acquire_acceleration_structure(AccelerationStructureType type, MemorySize size);
   void release_acceleration_structure(AccelerationStructure* as);

 private:
   Device& m_device;
   BufferHeap m_acc_struct_heap;
   std::map<const AccelerationStructure*, BufferHeap::Section> m_sections;
   std::multimap<MemorySize, AccelerationStructure*> m_free_acceleration_structures;
};

}// namespace triglav::graphics_api::ray_tracing
