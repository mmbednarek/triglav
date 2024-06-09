#include "TextureLoader.h"

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using triglav::graphics_api::SampleCount;
using triglav::graphics_api::TextureUsage;

namespace triglav::resource {

using namespace name_literals;

graphics_api::Texture Loader<ResourceType::Texture>::load_gpu(graphics_api::Device& device, const io::Path& path,
                                                              const ResourceProperties& props)
{
   int texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   assert(pixels != nullptr);

   auto texture = GAPI_CHECK(device.create_texture(
      GAPI_FORMAT(RGBA, sRGB), {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
      TextureUsage::Sampled | TextureUsage::TransferDst | TextureUsage::TransferSrc, SampleCount::Single, graphics_api::g_maxMipMaps));
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
