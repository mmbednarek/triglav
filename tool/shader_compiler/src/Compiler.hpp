#pragma once

#include "CommandLine.hpp"

#include "../../../library/engine/asset/include/triglav/asset/ShaderFormat.hpp"

#include <map>
#include <optional>
#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang.h>

namespace triglav::shader_compiler {

class Compiler
{
 public:
   explicit Compiler(Slang::ComPtr<slang::IGlobalSession> global_session, BuildType build_type, io::Path source_path,
                     std::vector<char> source_shader);

   static std::optional<Compiler> create_compiler(const io::Path& path, BuildType build_type);

   std::optional<asset::ShaderBlob> compile(const Target& target, asset::BytecodeFormat format,
                                            const std::map<std::string, std::string>& definitions,
                                            const std::vector<io::Path>& search_paths);
   void process_reflection(slang::ProgramLayout& layout);
   [[nodiscard]] std::vector<asset::ShaderProperty> get_properties() const;

 private:
   Slang::ComPtr<slang::IGlobalSession> m_global_session;
   BuildType m_build_type;
   std::map<std::string, asset::ShaderProperty> m_properties;
   io::Path m_source_path;
   std::vector<char> m_source_shader;
};

}// namespace triglav::shader_compiler