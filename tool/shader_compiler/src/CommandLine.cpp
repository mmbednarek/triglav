#include "CommandLine.hpp"

#include "triglav/Ranges.hpp"

#include <print>

namespace triglav::shader_compiler {

using namespace std::string_view_literals;

namespace {

std::array<std::string_view, PARAMETER_COUNT> PARAMETER_SHORT_FLAGS = {
   "-I"sv, "-b"sv, "-s"sv, "-o"sv, "-e"sv, "-D"sv,
};

std::array<std::string_view, PARAMETER_COUNT> PARAMETER_LONG_FLAGS = {
   "--library"sv, "--build"sv, "--stage"sv, "--output"sv, "--entry"sv, "--definition"sv,
};

std::pair<Parameter, std::string> parse_short_flag(const std::string_view input)
{
   for (const auto [index, flag] : Enumerate(PARAMETER_SHORT_FLAGS)) {
      if (input.starts_with(flag)) {
         return std::make_pair(static_cast<Parameter>(index), std::string(input.substr(flag.size())));
      }
   }
   return std::make_pair(Parameter::MaxValue, "");
};

Parameter parse_long_flag(const std::string_view input)
{
   for (const auto [index, flag] : Enumerate(PARAMETER_LONG_FLAGS)) {
      if (input == flag) {
         return static_cast<Parameter>(index);
      }
   }
   return Parameter::MaxValue;
};

BuildType parse_build_type(const std::string_view build_type_string)
{
   if (build_type_string == "release") {
      return BuildType::Release;
   }
   return BuildType::Debug;
}

render_core::ShaderStage parse_shader_stage(const std::string_view shader_stage_string)
{
   if (shader_stage_string == "fragment" || shader_stage_string == "pixel") {
      return render_core::ShaderStage::Pixel;
   }
   if (shader_stage_string == "vertex") {
      return render_core::ShaderStage::Vertex;
   }
   if (shader_stage_string == "domain") {
      return render_core::ShaderStage::Domain;
   }
   if (shader_stage_string == "hull") {
      return render_core::ShaderStage::Hull;
   }
   if (shader_stage_string == "compute") {
      return render_core::ShaderStage::Compute;
   }
   if (shader_stage_string == "ray_generation") {
      return render_core::ShaderStage::RayGeneration;
   }
   if (shader_stage_string == "any_hit") {
      return render_core::ShaderStage::AnyHit;
   }
   if (shader_stage_string == "closest_hit") {
      return render_core::ShaderStage::ClosestHit;
   }
   if (shader_stage_string == "miss") {
      return render_core::ShaderStage::Miss;
   }
   if (shader_stage_string == "intersection") {
      return render_core::ShaderStage::Intersection;
   }
   if (shader_stage_string == "callable") {
      return render_core::ShaderStage::Callable;
   }
   if (shader_stage_string == "task") {
      return render_core::ShaderStage::Task;
   }
   if (shader_stage_string == "mesh") {
      return render_core::ShaderStage::Mesh;
   }
   return render_core::ShaderStage::Fragment;
}

std::pair<std::string, std::string> parse_definition(const std::string_view input)
{
   const auto equal_pos = input.find('=');
   return std::make_pair(std::string(input.substr(0, equal_pos)), std::string(input.substr(equal_pos + 1)));
}

}// namespace

std::optional<ProgramArgs> parse_arguments(std::span<std::string_view> program_args)
{
   enum class State
   {
      General,
      ReadsValue,
   };

   ProgramArgs result{};
   State state = State::General;
   Parameter current_param = Parameter::MaxValue;
   Target current_target{};

   const auto set_param = [&](const Parameter param, const std::string_view value) {
      if (param == Parameter::TargetStage) {
         if (!current_target.destination_path.is_empty()) {
            result.targets.emplace_back(current_target);
         }
      }

      switch (param) {
      case Parameter::LibraryPath:
         result.library_paths.emplace_back(value);
         break;
      case Parameter::BuildType:
         result.build_type = parse_build_type(value);
         break;
      case Parameter::TargetStage:
         current_target.stage = parse_shader_stage(value);
         break;
      case Parameter::Output:
         current_target.destination_path = io::Path(value);
         break;
      case Parameter::EntryPoint:
         current_target.entry_point = value;
         break;
      case Parameter::Definition:
         result.definitions.insert(parse_definition(value));
      default:
         break;
      }
   };

   auto it = program_args.begin();
   while (it != program_args.end()) {
      const auto arg = *it;

      switch (state) {
      case State::General: {
         if (arg.starts_with("--")) {
            const auto param = parse_long_flag(arg);
            if (param == Parameter::MaxValue) {
               std::print(stdout, "shader_compiler: Unrecognized flag {}", arg);
               return std::nullopt;
            }
            state = State::ReadsValue;
            current_param = param;
         } else if (arg.starts_with("-")) {
            const auto [param, value] = parse_short_flag(arg);
            if (param == Parameter::MaxValue) {
               std::print(stdout, "shader_compiler: Unrecognized flag {}", arg);
               return std::nullopt;
            }
            if (!value.empty()) {
               set_param(param, value);
            } else {
               state = State::ReadsValue;
               current_param = param;
            }
         } else {
            if (!result.source_shader.is_empty()) {
               std::print(stdout, "shader_compiler: Failed to parse args, multiple source files are not supported");
               return std::nullopt;
            }
            result.source_shader = io::Path{arg};
         }
         break;
      }
      case State::ReadsValue: {
         set_param(current_param, arg);
         state = State::General;
         break;
      }
      }

      ++it;
   }

   if (!current_target.destination_path.is_empty()) {
      result.targets.emplace_back(current_target);
   }

   return result;
}

}// namespace triglav::shader_compiler