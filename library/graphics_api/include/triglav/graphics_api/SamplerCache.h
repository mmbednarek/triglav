#pragma once

#include "GraphicsApi.hpp"
#include "Sampler.h"

#include "triglav/Int.hpp"

#include <map>

namespace triglav::graphics_api {

class Device;

class SamplerCache {
 public:
   using Hash = u64;

   explicit SamplerCache(Device &device);

   const Sampler& find_sampler(const SamplerProperties& properties);

 private:
   Device &m_device;
   std::map<Hash, Sampler> m_samplers;
};

}