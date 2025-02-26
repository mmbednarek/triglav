#include "TextureLoader.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Texture.hpp"
#include "triglav/io/File.hpp"

namespace triglav::resource {

using graphics_api::SampleCount;
using graphics_api::TextureState;
using graphics_api::TextureUsage;

using namespace name_literals;

graphics_api::Texture Loader<ResourceType::Texture>::load_gpu(graphics_api::Device& device, [[maybe_unused]] const TextureName name,
                                                              const io::Path& path, const ResourceProperties& /*props*/)
{
   auto file = io::open_file(path, io::FileOpenMode::Read);
   assert(file.has_value());

   [[maybe_unused]]
   const auto header = asset::decode_header(**file);
   assert(header.has_value());
   assert(header->type == ResourceType::Texture);

   const auto decodedTex = asset::decode_texture(**file);
   assert(decodedTex.has_value());

   auto texture = GAPI_CHECK(device.create_texture_from_ktx(
      decodedTex->texture, TextureUsage::Sampled | TextureUsage::TransferDst | TextureUsage::TransferSrc, TextureState::ShaderRead));

   const auto& sampler = decodedTex->samplerProps;
   texture.sampler_properties().minFilter = static_cast<graphics_api::FilterType>(sampler.minFilter);
   texture.sampler_properties().magFilter = static_cast<graphics_api::FilterType>(sampler.magFilter);
   texture.sampler_properties().addressU = static_cast<graphics_api::TextureAddressMode>(sampler.addressModeU);
   texture.sampler_properties().addressV = static_cast<graphics_api::TextureAddressMode>(sampler.addressModeV);
   texture.sampler_properties().addressW = static_cast<graphics_api::TextureAddressMode>(sampler.addressModeW);
   texture.sampler_properties().enableAnisotropy = sampler.enableAnisotropy;
   texture.sampler_properties().minLod = 0.0f;
   texture.sampler_properties().maxLod = static_cast<float>(texture.mip_count());
   texture.sampler_properties().mipBias = 0.0f;

   return texture;
}

}// namespace triglav::resource
