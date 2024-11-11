#include "SamplerCache.hpp"

#include <bit>

#include "Device.hpp"

namespace triglav::graphics_api {

namespace {

SamplerCache::Hash calculate_hash(const SamplerProperties& properties)
{
   SamplerCache::Hash result{};
   result += 7011713ULL * static_cast<u64>(properties.minFilter);
   result += 3424423ULL * static_cast<u64>(properties.magFilter);
   result += 2569937ULL * static_cast<u64>(properties.addressU);
   result += 3703781ULL * static_cast<u64>(properties.addressV);
   result += 6402203ULL * static_cast<u64>(properties.addressU);
   result += 4184507ULL * static_cast<u64>(properties.enableAnisotropy);
   result += 8753531ULL * std::bit_cast<u32>(properties.mipBias);
   result += 6532073ULL * std::bit_cast<u32>(properties.minLod);
   result += 5774761ULL * std::bit_cast<u32>(properties.maxLod);
   return result;
}

}// namespace

SamplerCache::SamplerCache(Device& device) :
    m_device(device)
{
}

const Sampler& SamplerCache::find_sampler(const SamplerProperties& properties)
{
   const auto hash = calculate_hash(properties);
   const auto it = m_samplers.find(hash);
   if (it != m_samplers.end()) {
      return it->second;
   }

   auto [samplerIt, ok] = m_samplers.emplace(hash, GAPI_CHECK(m_device.create_sampler(properties)));
   assert(ok);

   return samplerIt->second;
}

}// namespace triglav::graphics_api
