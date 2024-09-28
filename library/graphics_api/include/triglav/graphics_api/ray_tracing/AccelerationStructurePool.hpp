#pragma once

#include "RayTracing.hpp"

#include "triglav/Int.hpp"
#include "triglav/ObjectPool.hpp"

#include <memory>
#include <set>

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
   void release_acceleration_structure(AccelerationStructure* as);

   Buffer* allocate_scratch_buffer(MemorySize size);
   void release_scratch_buffer(Buffer* buff);

 private:
  Page& allocate_page();
  std::vector<Page>::iterator find_available_page(std::vector<Page>::iterator start, Index chunkCount);

   Device& m_device;
   std::vector<Page> m_pages;
   std::map<AccelerationStructure*, Node*> m_asToNodeMap;
   std::set<Buffer*> m_scratchBuffers;
};

}// namespace triglav::graphics_api::ray_tracing
