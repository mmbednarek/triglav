#include "SamplerCache.hpp"

#include <bit>

#include "Device.hpp"

namespace triglav::graphics_api {

namespace {

SamplerCache::Hash calculate_hash(const SamplerProperties& properties)
{
   SamplerCache::Hash result{};
   result += 7011713ULL * static_cast<u64>(properties.min_filter);
   result += 3424423ULL * static_cast<u64>(properties.mag_filter);
   result += 2569937ULL * static_cast<u64>(properties.address_u);
   result += 3703781ULL * static_cast<u64>(properties.address_v);
   result += 6402203ULL * static_cast<u64>(properties.address_u);
   result += 4184507ULL * static_cast<u64>(properties.enable_anisotropy);
   result += 8753531ULL * std::bit_cast<u32>(properties.mip_bias);
   result += 6532073ULL * std::bit_cast<u32>(properties.min_lod);
   result += 5774761ULL * std::bit_cast<u32>(properties.max_lod);
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

   auto [sampler_it, ok] = m_samplers.emplace(hash, GAPI_CHECK(m_device.create_sampler(properties)));
   assert(ok);

   return sampler_it->second;
}

}// namespace triglav::graphics_api
