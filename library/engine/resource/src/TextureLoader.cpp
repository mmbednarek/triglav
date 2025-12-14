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
                                                              const io::Path& path)
{
   auto file = io::open_file(path, io::FileMode::Read);
   assert(file.has_value());

   [[maybe_unused]]
   const auto header = asset::decode_header(**file);
   assert(header.has_value());
   assert(header->type == ResourceType::Texture);

   const auto decoded_tex = asset::decode_texture(**file);
   assert(decoded_tex.has_value());

   auto texture = GAPI_CHECK(device.create_texture_from_ktx(
      decoded_tex->texture, TextureUsage::Sampled | TextureUsage::TransferDst | TextureUsage::TransferSrc, TextureState::ShaderRead));

   const auto& sampler = decoded_tex->sampler_props;
   texture.sampler_properties().min_filter = static_cast<graphics_api::FilterType>(sampler.min_filter);
   texture.sampler_properties().mag_filter = static_cast<graphics_api::FilterType>(sampler.mag_filter);
   texture.sampler_properties().address_u = static_cast<graphics_api::TextureAddressMode>(sampler.address_mode_u);
   texture.sampler_properties().address_v = static_cast<graphics_api::TextureAddressMode>(sampler.address_mode_v);
   texture.sampler_properties().address_w = static_cast<graphics_api::TextureAddressMode>(sampler.address_mode_w);
   texture.sampler_properties().enable_anisotropy = sampler.enable_anisotropy;
   texture.sampler_properties().min_lod = 0.0f;
   texture.sampler_properties().max_lod = static_cast<float>(texture.mip_count());
   texture.sampler_properties().mip_bias = 0.0f;

   return texture;
}

}// namespace triglav::resource
