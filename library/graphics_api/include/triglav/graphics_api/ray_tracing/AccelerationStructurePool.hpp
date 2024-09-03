#pragma once

#include "AccelerationStructure.hpp"

#include "triglav/Int.hpp"
#include "triglav/ObjectPool.hpp"

#include <vector>

namespace triglav::graphics_api::ray_tracing {

class AccelerationStructurePool {
 public:
   explicit AccelerationStructurePool(Device& device);

   AccelerationStructure* acquire_acceleration_structure(AccelerationStructureType type, MemorySize size);
   void release_acceleration_structure(AccelerationStructure* as);

 private:
   Device& m_device;
   std::map<AccelerationStructure*, Buffer*> m_asToBufferMap;
};

}
