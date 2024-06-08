#include "StaticResources.h"

#include "ResourceManager.h"

#include "triglav/graphics_api/Device.h"

namespace triglav::resource {

using namespace name_literals;

void register_samplers(graphics_api::Device& device, ResourceManager& manager)
{
   using graphics_api::FilterType;
   using graphics_api::SamplerProperties;
   using graphics_api::TextureAddressMode;

   SamplerProperties info{};

   info.magFilter = FilterType::Linear;
   info.minFilter = FilterType::Linear;
   info.addressU = TextureAddressMode::Repeat;
   info.addressV = TextureAddressMode::Repeat;
   info.addressW = TextureAddressMode::Repeat;
   info.enableAnisotropy = true;
   info.mipBias = 0.0f;
   info.minLod = 0.0f;
   info.maxLod = 0.0f;
   manager.emplace_resource<ResourceType::Sampler>("linear_repeat_mlod0_aniso.sampler"_rc, GAPI_CHECK(device.create_sampler(info)));

   info.enableAnisotropy = false;
   manager.emplace_resource<ResourceType::Sampler>("linear_repeat_mlod0.sampler"_rc, GAPI_CHECK(device.create_sampler(info)));

   info.enableAnisotropy = true;
   info.maxLod = 8.0f;
   manager.emplace_resource<ResourceType::Sampler>("linear_repeat_mlod8_aniso.sampler"_rc, GAPI_CHECK(device.create_sampler(info)));

   info.enableAnisotropy = true;
   info.mipBias = 0.0f;
   info.minLod = 0.0f;
   info.maxLod = 8.0f;
   manager.emplace_resource<ResourceType::Sampler>("ground_sampler.sampler"_rc, GAPI_CHECK(device.create_sampler(info)));
}

}// namespace triglav::resource