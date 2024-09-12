#pragma once

#include "RayTracing.hpp"

#include "triglav/Int.hpp"
#include "triglav/ObjectPool.hpp"

#include <vector>

namespace triglav::graphics_api {
class Device;
class Buffer;
}

namespace triglav::graphics_api::ray_tracing {

class AccelerationStructure;

class AccelerationStructurePool {
 public:
   explicit AccelerationStructurePool(Device& device);

   AccelerationStructure* acquire_acceleration_structure(AccelerationStructureType type, MemorySize size);
   void release_acceleration_structure(AccelerationStructure* as);

   Buffer* allocate_scratch_buffer(MemorySize size);
   void release_scratch_buffer(Buffer* buff);

 private:
   Device& m_device;
   std::map<AccelerationStructure*, Buffer*> m_asToBufferMap;
};

}
