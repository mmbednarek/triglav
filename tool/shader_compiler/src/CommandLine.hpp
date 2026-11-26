#pragma once

#include "triglav/io/Path.hpp"
#include "triglav/render_core/RenderCore.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace triglav::shader_compiler {

// shader_compiler source.slang -I ../library --build release --target vertex -o out dest.vshader --target fragment -o dest.fshader

enum class Parameter
{
   LibraryPath,
   BuildType,
   TargetStage,
   Output,
   EntryPoint,
   Definition,
   MaxValue,
};

static constexpr u32 PARAMETER_COUNT = static_cast<u32>(Parameter::MaxValue);

struct Target
{
   io::Path destination_path;
   render_core::ShaderStage stage;
   std::string entry_point;
};

enum class BuildType
{
   Debug,
   Release,
};

struct ProgramArgs
{
   io::Path source_shader;
   std::vector<Target> targets;
   BuildType build_type = BuildType::Debug;
   std::vector<io::Path> library_paths;
   std::map<std::string, std::string> definitions;
};

std::optional<ProgramArgs> parse_arguments(std::span<std::string_view> program_args);

}// namespace triglav::shader_compiler
