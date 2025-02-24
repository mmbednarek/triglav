#include "TextureLoader.hpp"

#include "triglav/NameResolution.hpp"
#include "triglav/asset/Asset.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Texture.hpp"
#include "triglav/io/File.hpp"
#include "triglav/ktx/Texture.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace triglav::resource {

using graphics_api::SampleCount;
using graphics_api::TextureState;
using graphics_api::TextureUsage;

using namespace name_literals;

graphics_api::Texture load_tex_texture(graphics_api::Device& device, const io::Path& path)
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

graphics_api::Texture load_ktx_texture(graphics_api::Device& device, const io::Path& path)
{
   const auto ktxTexture = ktx::Texture::from_file(path);
   assert(ktxTexture.has_value());

   return GAPI_CHECK(device.create_texture_from_ktx(
      *ktxTexture, TextureUsage::Sampled | TextureUsage::TransferDst | TextureUsage::TransferSrc, TextureState::ShaderRead));
}

graphics_api::Texture Loader<ResourceType::Texture>::load_gpu(graphics_api::Device& device, [[maybe_unused]] const TextureName name,
                                                              const io::Path& path, const ResourceProperties& props)
{
   if (path.string().ends_with(".tex")) {
      return load_tex_texture(device, path);
   }
   if (path.string().ends_with(".ktx")) {
      return load_ktx_texture(device, path);
   }

   int texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   assert(pixels != nullptr);

   auto texture =
      GAPI_CHECK(device.create_texture(GAPI_FORMAT(RGBA, sRGB), {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
                                       TextureUsage::Sampled | TextureUsage::TransferDst | TextureUsage::TransferSrc,
                                       TextureState::Undefined, SampleCount::Single, graphics_api::g_maxMipMaps));

   TG_SET_DEBUG_NAME(texture, resolve_name(name.name()));

   GAPI_CHECK_STATUS(texture.write(device, pixels));

   stbi_image_free(pixels);

   texture.set_anisotropy_state(props.get_bool("anisotropy"_name, true));
   auto maxLod = props.get_float_opt("max_lod"_name);
   if (maxLod.has_value()) {
      texture.set_lod(0.0f, *maxLod);
   }

   return texture;
}

}// namespace triglav::resource
