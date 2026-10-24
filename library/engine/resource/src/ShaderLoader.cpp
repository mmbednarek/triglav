#include "ShaderLoader.hpp"

#include "triglav/NameResolution.hpp"
#include "triglav/ResourcePathMap.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/io/File.hpp"

namespace triglav::resource {

graphics_api::Shader Loader<ResourceType::FragmentShader>::load_gpu(graphics_api::Device& device,
                                                                    [[maybe_unused]] const FragmentShaderName name, const io::Path& path)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::FragmentShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::VertexShader>::load_gpu(graphics_api::Device& device,
                                                                  [[maybe_unused]] const VertexShaderName name, const io::Path& path)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::VertexShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::HullShader>::load_gpu(graphics_api::Device& device, [[maybe_unused]] const HullShaderName name,
                                                                const io::Path& path)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::HullShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::DomainShader>::load_gpu(graphics_api::Device& device,
                                                                  [[maybe_unused]] const DomainShaderName name, const io::Path& path)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::DomainShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::ComputeShader>::load_gpu(graphics_api::Device& device,
                                                                   [[maybe_unused]] const ComputeShaderName name, const io::Path& path)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::ComputeShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::RayGenShader>::load_gpu(graphics_api::Device& device,
                                                                  [[maybe_unused]] const RayGenShaderName name, const io::Path& path)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::RayGenerationShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::RayClosestHitShader>::load_gpu(graphics_api::Device& device,
                                                                         [[maybe_unused]] const RayClosestHitShaderName name,
                                                                         const io::Path& path)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::ClosestHitShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::RayMissShader>::load_gpu(graphics_api::Device& device,
                                                                   [[maybe_unused]] const RayMissShaderName name, const io::Path& path)
{
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::MissShader, "main", io::read_whole_file(path)));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

}// namespace triglav::resource
