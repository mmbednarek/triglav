#include "ShaderLoader.hpp"

#include "triglav/ResourcePathMap.hpp"
#include "triglav/asset/Asset.hpp"
#include "triglav/asset/ShaderFormat.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/io/File.hpp"
#include "triglav/meta/Binary.hpp"

namespace triglav::resource {

namespace {

std::vector<u8> load_spirv(const io::Path& path, const ResourceType expected_res_type)
{
   auto file = io::open_file(path, io::FileMode::Read);
   const auto header = asset::decode_header(**file);
   if (!header.has_value())
      return {};

   if (header->type != expected_res_type)
      return {};

   asset::ShaderData shader_data;
   meta::deserialize_binary(**file, shader_data.to_meta_ref());

   for (auto& blob : shader_data.blobs) {
      if (blob.format != asset::BytecodeFormat::SPIRV)
         continue;

      return std::move(blob.data);
   }

   return {};
}

}// namespace

graphics_api::Shader Loader<ResourceType::FragmentShader>::load_gpu(graphics_api::Device& device,
                                                                    [[maybe_unused]] const FragmentShaderName name, const io::Path& path)
{
   const auto spirv = load_spirv(path, ResourceType::FragmentShader);
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::FragmentShader, "main", spirv));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::VertexShader>::load_gpu(graphics_api::Device& device,
                                                                  [[maybe_unused]] const VertexShaderName name, const io::Path& path)
{
   const auto spirv = load_spirv(path, ResourceType::VertexShader);
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::VertexShader, "main", spirv));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::HullShader>::load_gpu(graphics_api::Device& device, [[maybe_unused]] const HullShaderName name,
                                                                const io::Path& path)
{
   const auto spirv = load_spirv(path, ResourceType::HullShader);
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::HullShader, "main", spirv));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::DomainShader>::load_gpu(graphics_api::Device& device,
                                                                  [[maybe_unused]] const DomainShaderName name, const io::Path& path)
{
   const auto spirv = load_spirv(path, ResourceType::DomainShader);
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::DomainShader, "main", spirv));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::ComputeShader>::load_gpu(graphics_api::Device& device,
                                                                   [[maybe_unused]] const ComputeShaderName name, const io::Path& path)
{
   const auto spirv = load_spirv(path, ResourceType::ComputeShader);
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::ComputeShader, "main", spirv));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::RayGenShader>::load_gpu(graphics_api::Device& device,
                                                                  [[maybe_unused]] const RayGenShaderName name, const io::Path& path)
{
   const auto spirv = load_spirv(path, ResourceType::RayGenShader);
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::RayGenerationShader, "main", spirv));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::RayClosestHitShader>::load_gpu(graphics_api::Device& device,
                                                                         [[maybe_unused]] const RayClosestHitShaderName name,
                                                                         const io::Path& path)
{
   const auto spirv = load_spirv(path, ResourceType::RayClosestHitShader);
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::ClosestHitShader, "main", spirv));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

graphics_api::Shader Loader<ResourceType::RayMissShader>::load_gpu(graphics_api::Device& device,
                                                                   [[maybe_unused]] const RayMissShaderName name, const io::Path& path)
{
   const auto spirv = load_spirv(path, ResourceType::RayMissShader);
   auto shader = GAPI_CHECK(device.create_shader(graphics_api::PipelineStage::MissShader, "main", spirv));
   TG_SET_DEBUG_NAME(shader, ResourcePathMap::the().resolve(name).to_std());
   return shader;
}

}// namespace triglav::resource
