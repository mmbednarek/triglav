#pragma once

#include "../BufferHeap.hpp"
#include "RayTracing.hpp"

#include "triglav/Int.hpp"
#include "triglav/ObjectPool.hpp"

#include <memory>

namespace triglav::graphics_api {
class Device;
class Buffer;
}// namespace triglav::graphics_api

namespace triglav::graphics_api::ray_tracing {

class AccelerationStructure;

class AccelerationStructurePool
{
 public:
   struct Node
   {
      Index chunkCount;
      bool isFree;
      Node* next;
   };

   struct Page
   {
      std::unique_ptr<Buffer> buffer;
      Index availableChunks;
      Node* headNode;
   };

   explicit AccelerationStructurePool(Device& device);
   ~AccelerationStructurePool();

   AccelerationStructure* acquire_acceleration_structure(AccelerationStructureType type, MemorySize size);
   void release_acceleration_structure(const AccelerationStructure* as);

 private:
   Device& m_device;
   BufferHeap m_accStructHeap;
   std::map<const AccelerationStructure*, BufferHeap::Section> m_sections;
};

}// namespace triglav::graphics_api::ray_tracing
