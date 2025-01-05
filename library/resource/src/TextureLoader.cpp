#include "TextureLoader.hpp"

#include "triglav/NameResolution.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace triglav::resource {

using graphics_api::SampleCount;
using graphics_api::TextureState;
using graphics_api::TextureUsage;

using namespace name_literals;

graphics_api::Texture Loader<ResourceType::Texture>::load_gpu(graphics_api::Device& device, [[maybe_unused]] const TextureName name,
                                                              const io::Path& path, const ResourceProperties& props)
{
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
