#include "CommandLine.hpp"
#include "Compiler.hpp"
#include "triglav/asset/Asset.hpp"
#include "triglav/io/File.hpp"
#include "triglav/meta/Binary.hpp"

#include "triglav/asset/ShaderFormat.hpp"
#include "triglav/render_core/RenderCore.hpp"

#include <cassert>
#include <print>
#include <vector>

namespace {

triglav::ResourceType stage_to_resource_type(const triglav::render_core::ShaderStage stage)
{
   switch (stage) {
   case triglav::render_core::ShaderStage::Vertex:
      return triglav::ResourceType::VertexShader;
   case triglav::render_core::ShaderStage::Domain:
      return triglav::ResourceType::DomainShader;
   case triglav::render_core::ShaderStage::Hull:
      return triglav::ResourceType::HullShader;
   case triglav::render_core::ShaderStage::Fragment:
      return triglav::ResourceType::FragmentShader;
   case triglav::render_core::ShaderStage::Compute:
      return triglav::ResourceType::ComputeShader;
   default:
      return {};
   }
}

}// namespace

int main(int argc, const char** argv)
{
   std::vector<std::string_view> program_args_vec(argc - 1);
   for (int i = 1; i < argc; ++i) {
      program_args_vec[i - 1] = std::string_view(argv[i]);
   }

   const auto program_args = triglav::shader_compiler::parse_arguments(program_args_vec);
   if (!program_args.has_value()) {
      return EXIT_FAILURE;
   }

   if (!program_args->source_shader.exists()) {
      std::print(stderr, "shader_compiler: Failed to open shader file: \"{}\"\n", program_args->source_shader.string());
      return EXIT_FAILURE;
   }

   std::optional<triglav::shader_compiler::Compiler> compiler =
      triglav::shader_compiler::Compiler::create_compiler(program_args->source_shader, program_args->build_type);
   if (!compiler.has_value()) {
      return EXIT_FAILURE;
   }

   for (const auto& target : program_args->targets) {
      triglav::asset::ShaderData shader_data;

      auto spirv_blob =
         compiler->compile(target, triglav::asset::BytecodeFormat::SPIRV, program_args->definitions, program_args->library_paths);
      if (!spirv_blob.has_value()) {
         return EXIT_FAILURE;
      }

      shader_data.blobs.emplace_back(std::move(*spirv_blob));

      const auto file = triglav::io::open_file(target.destination_path, triglav::io::FileMode::Write | triglav::io::FileMode::Create);
      if (!file.has_value()) {
         std::print(stderr, "shader_compiler: Failed to open output file\n");
         return EXIT_FAILURE;
      }

      shader_data.properties = compiler->get_properties();

      triglav::asset::write_header(**file, stage_to_resource_type(target.stage));
      triglav::meta::serialize_binary(**file, shader_data.to_meta_ref());
   }

   return EXIT_SUCCESS;
}