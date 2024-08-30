#include "ShaderLoader.h"

#include "triglav/graphics_api/Device.hpp"
#include "triglav/io/File.h"

namespace triglav::resource {

graphics_api::Shader Loader<ResourceType::FragmentShader>::load_gpu(graphics_api::Device& device, const io::Path& path,
                                                                    const ResourceProperties& props)
{
   return GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::FragmentShader, "main", io::read_whole_file(path)));
}

graphics_api::Shader Loader<ResourceType::VertexShader>::load_gpu(graphics_api::Device& device, const io::Path& path,
                                                                  const ResourceProperties& props)
{
   return GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::VertexShader, "main", io::read_whole_file(path)));
}

graphics_api::Shader Loader<ResourceType::ComputeShader>::load_gpu(graphics_api::Device& device, const io::Path& path,
                                                                   const ResourceProperties& props)
{
   return GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::ComputeShader, "main", io::read_whole_file(path)));
}

}// namespace triglav::resource
