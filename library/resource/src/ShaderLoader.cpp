#include "ShaderLoader.hpp"

#include "triglav/NameResolution.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/io/File.hpp"

namespace triglav::resource {

graphics_api::Shader Loader<ResourceType::FragmentShader>::load_gpu(graphics_api::Device& device, const FragmentShaderName /*name*/,
                                                                    const io::Path& path, const ResourceProperties& /*props*/)
{
   return GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::FragmentShader, "main", io::read_whole_file(path)));
}

graphics_api::Shader Loader<ResourceType::VertexShader>::load_gpu(graphics_api::Device& device, const VertexShaderName name,
                                                                  const io::Path& path, const ResourceProperties& /*props*/)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::VertexShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, resolve_name(name.name()));
   return shader;
}

graphics_api::Shader Loader<ResourceType::ComputeShader>::load_gpu(graphics_api::Device& device, const ComputeShaderName /*name*/,
                                                                   const io::Path& path, const ResourceProperties& /*props*/)
{
   return GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::ComputeShader, "main", io::read_whole_file(path)));
}

graphics_api::Shader Loader<ResourceType::RayGenShader>::load_gpu(graphics_api::Device& device, const RayGenShaderName /*name*/,
                                                                  const io::Path& path, const ResourceProperties& /*props*/)
{
   return GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::RayGenerationShader, "main", io::read_whole_file(path)));
}

graphics_api::Shader Loader<ResourceType::RayClosestHitShader>::load_gpu(graphics_api::Device& device,
                                                                         const RayClosestHitShaderName /*name*/, const io::Path& path,
                                                                         const ResourceProperties& /*props*/)
{
   return GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::ClosestHitShader, "main", io::read_whole_file(path)));
}

graphics_api::Shader Loader<ResourceType::RayMissShader>::load_gpu(graphics_api::Device& device, const RayMissShaderName /*name*/,
                                                                   const io::Path& path, const ResourceProperties& /*props*/)
{
   return GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::MissShader, "main", io::read_whole_file(path)));
}

}// namespace triglav::resource
