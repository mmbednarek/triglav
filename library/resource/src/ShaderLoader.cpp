#include "ShaderLoader.h"

#include "graphics_api/Device.h"

#include <fstream>

namespace {

std::vector<uint8_t> read_whole_file(const std::string_view name)
{
   std::ifstream file(std::string{name}, std::ios::ate | std::ios::binary);
   if (not file.is_open()) {
      return {};
   }

   file.seekg(0, std::ios::end);
   const auto fileSize = file.tellg();
   file.seekg(0, std::ios::beg);

   std::vector<uint8_t> result{};
   result.resize(fileSize);

   file.read(reinterpret_cast<char *>(result.data()), fileSize);
   return result;
}

graphics_api::Shader load_shader(graphics_api::Device &device, const graphics_api::ShaderStage type,
                                 const std::string_view path)
{
   return GAPI_CHECK(device.create_shader(type, "main", read_whole_file(path)));
}

}// namespace

namespace triglav::resource {

graphics_api::Shader Loader<ResourceType::FragmentShader>::load_gpu(graphics_api::Device &device,
                                                                    const std::string_view path)
{
   return load_shader(device, graphics_api::ShaderStage::Fragment, path);
}

graphics_api::Shader Loader<ResourceType::VertexShader>::load_gpu(graphics_api::Device &device,
                                                                  const std::string_view path)
{
   return load_shader(device, graphics_api::ShaderStage::Vertex, path);
}

}// namespace triglav::resource
