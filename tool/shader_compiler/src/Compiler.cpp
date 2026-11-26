#include "Compiler.hpp"

#include "triglav/asset/ShaderFormat.hpp"
#include "triglav/io/File.hpp"

#include <print>
#include <ranges>
#include <utility>

namespace triglav::shader_compiler {

using namespace std::string_view_literals;

namespace {

template<typename TDerived>
class RefCounted : public TDerived
{
 public:
   virtual ~RefCounted() = default;

   SlangResult queryInterface(const SlangUUID& uuid, void** outObject) override
   {
      if (outObject == nullptr)
         return SLANG_E_INVALID_ARG;

      if (uuid == TDerived::getTypeGuid()) {
         this->addRef();
         *outObject = static_cast<TDerived*>(this);
         return SLANG_OK;
      }
      if (uuid == SLANG_UUID_ISlangUnknown) {
         this->addRef();
         *outObject = static_cast<ISlangUnknown*>(this);
         return SLANG_OK;
      }

      *outObject = nullptr;
      return SLANG_E_NO_INTERFACE;
   }

   uint32_t addRef() override
   {
      return m_ref_count.fetch_add(1, std::memory_order_relaxed) + 1;
   }

   uint32_t release() override
   {
      const u32 count = m_ref_count.fetch_sub(1, std::memory_order_acq_rel) - 1;
      if (count == 0)
         delete this;
      return count;
   }

 private:
   std::atomic_int32_t m_ref_count = 1;
};

class MemoryBlob final : public RefCounted<slang::IBlob>
{
 public:
   explicit MemoryBlob(std::vector<char> data) :
       m_data(std::move(data))
   {
   }

   const void* getBufferPointer() override
   {
      return m_data.data();
   }

   size_t getBufferSize() override
   {
      return m_data.size();
   }

 private:
   std::vector<char> m_data;
};

class FileSystem final : public RefCounted<ISlangFileSystem>
{
 public:
   FileSystem() = default;

   void* castAs(const SlangUUID& guid) override
   {
      if (guid == ISlangFileSystem::getTypeGuid())
         return static_cast<ISlangFileSystem*>(this);
      if (guid == ISlangCastable::getTypeGuid())
         return static_cast<ISlangCastable*>(this);
      if (guid == ISlangUnknown::getTypeGuid())
         return static_cast<ISlangUnknown*>(this);
      return nullptr;
   }

   SlangResult loadFile(const char* path, ISlangBlob** outBlob) override
   {
      auto buffer = io::read_whole_file(io::Path{path});
      if (buffer.empty()) {
         return SLANG_E_NOT_FOUND;
      }
      *outBlob = new MemoryBlob(std::move(buffer));
      return SLANG_OK;
   }
};

const char* get_profile(asset::BytecodeFormat bc_format, render_core::ShaderStage stage)
{
   if (bc_format == asset::BytecodeFormat::SPIRV) {
      return "spirv_1_6+spvImageQuery+spvSparseResidency";
   }

   if (bc_format == asset::BytecodeFormat::DXIL) {
      switch (stage) {
      case render_core::ShaderStage::Vertex:
         return "vs_6_7";
      case render_core::ShaderStage::Domain:
         return "ds_6_7";
      case render_core::ShaderStage::Hull:
         return "hs_6_7";
      case render_core::ShaderStage::Geometry:
         return "gs_6_7";
      case render_core::ShaderStage::Fragment:
         return "ps_6_7";
      case render_core::ShaderStage::Compute:
         return "cs_6_7";
      default:
         break;
      }
   }

   return "";
}

const char* get_entry_point(const Target& target)
{
   if (!target.entry_point.empty())
      return target.entry_point.data();

   switch (target.stage) {
   case render_core::ShaderStage::Vertex:
      return "vs_main";
   case render_core::ShaderStage::Domain:
      return "ds_main";
   case render_core::ShaderStage::Hull:
      return "hs_main";
   case render_core::ShaderStage::Geometry:
      return "gs_main";
   case render_core::ShaderStage::Fragment:
      return "fs_main";
   case render_core::ShaderStage::Compute:
      return "cs_main";
   case render_core::ShaderStage::MaxValue:
   default:
      return "shader_main";
   }
}

void print_diagnostic_blob(slang::IBlob& blob)
{
   std::span span_blob{static_cast<const u8*>(blob.getBufferPointer()), blob.getBufferSize()};
   [[maybe_unused]] const auto res = io::stderr_writer().write(span_blob);
   assert(res.has_value());
}

[[maybe_unused]]
const char* stage_to_string(const render_core::ShaderStage stage)
{
   switch (stage) {
   case render_core::ShaderStage::Vertex:
      return "vertex";
   case render_core::ShaderStage::Domain:
      return "domain";
   case render_core::ShaderStage::Hull:
      return "hull";
   case render_core::ShaderStage::Geometry:
      return "geometry";
   case render_core::ShaderStage::Fragment:
      return "fragment";
   case render_core::ShaderStage::Compute:
      return "compute";
   case render_core::ShaderStage::RayGeneration:
      return "ray_generation";
   case render_core::ShaderStage::AnyHit:
      return "any_hit";
   case render_core::ShaderStage::ClosestHit:
      return "closest_hit";
   case render_core::ShaderStage::Miss:
      return "miss";
   case render_core::ShaderStage::Intersection:
      return "intersection";
   case render_core::ShaderStage::Callable:
      return "callable";
   case render_core::ShaderStage::Task:
      return "task";
   case render_core::ShaderStage::Mesh:
      return "mesh";
   default:
      return "";
   }
}

SlangStage to_slang_stage(const render_core::ShaderStage stage)
{
   switch (stage) {
   case render_core::ShaderStage::Vertex:
      return SLANG_STAGE_VERTEX;
   case render_core::ShaderStage::Domain:
      return SLANG_STAGE_DOMAIN;
   case render_core::ShaderStage::Hull:
      return SLANG_STAGE_HULL;
   case render_core::ShaderStage::Geometry:
      return SLANG_STAGE_GEOMETRY;
   case render_core::ShaderStage::Fragment:
      return SLANG_STAGE_FRAGMENT;
   case render_core::ShaderStage::Compute:
      return SLANG_STAGE_COMPUTE;
   case render_core::ShaderStage::RayGeneration:
      return SLANG_STAGE_RAY_GENERATION;
   case render_core::ShaderStage::AnyHit:
      return SLANG_STAGE_ANY_HIT;
   case render_core::ShaderStage::ClosestHit:
      return SLANG_STAGE_CLOSEST_HIT;
   case render_core::ShaderStage::Miss:
      return SLANG_STAGE_MISS;
   case render_core::ShaderStage::Intersection:
      return SLANG_STAGE_INTERSECTION;
   case render_core::ShaderStage::Callable:
      return SLANG_STAGE_CALLABLE;
   case render_core::ShaderStage::Task:
      return SLANG_STAGE_AMPLIFICATION;
   case render_core::ShaderStage::Mesh:
      return SLANG_STAGE_MESH;
   default:
      return SLANG_STAGE_NONE;
   }
}

asset::ShaderPropertyType to_shader_property_type(const slang::ParameterCategory category)
{
   switch (category) {
   case slang::ParameterCategory::ConstantBuffer:
      return asset::ShaderPropertyType::ConstantBuffer;
   case slang::ParameterCategory::UnorderedAccess:
      return asset::ShaderPropertyType::RWStructuredBuffer;
   case slang::ParameterCategory::SamplerState:
      return asset::ShaderPropertyType::SamplerState;
   case slang::ParameterCategory::Uniform:
      return asset::ShaderPropertyType::Texture2D;
   default:
      return {};
   }
}

SlangOptimizationLevel build_type_to_optimization_level(const BuildType build)
{
   switch (build) {
   case BuildType::Debug:
      return SLANG_OPTIMIZATION_LEVEL_NONE;
   case BuildType::Release:
      return SLANG_OPTIMIZATION_LEVEL_MAXIMAL;
   }
   return SLANG_OPTIMIZATION_LEVEL_DEFAULT;
}

SlangDebugInfoLevel build_type_to_debug_information_level(const BuildType build)
{
   switch (build) {
   case BuildType::Debug:
      return SLANG_DEBUG_INFO_LEVEL_MAXIMAL;
   case BuildType::Release:
      return SLANG_DEBUG_INFO_LEVEL_NONE;
   }
   return SLANG_DEBUG_INFO_LEVEL_STANDARD;
}

}// namespace

Compiler::Compiler(Slang::ComPtr<slang::IGlobalSession> global_session, const BuildType build_type, io::Path source_path,
                   std::vector<char> source_shader) :
    m_global_session(std::move(global_session)),
    m_build_type(build_type),
    m_source_path(std::move(source_path)),
    m_source_shader(std::move(source_shader))
{
   m_source_shader.emplace_back(0);
}

std::optional<Compiler> Compiler::create_compiler(const io::Path& path, const BuildType build_type)
{
   Slang::ComPtr<slang::IGlobalSession> global_session;
   SlangGlobalSessionDesc slang_desc{};

   const auto result = slang::createGlobalSession(&slang_desc, global_session.writeRef());
   if (SLANG_FAILED(result)) {
      return std::nullopt;
   }

   return std::make_optional<Compiler>(std::move(global_session), build_type, path, io::read_whole_file(path));
}

std::optional<asset::ShaderBlob> Compiler::compile(const Target& target, const asset::BytecodeFormat format,
                                                   const std::map<std::string, std::string>& definitions,
                                                   const std::vector<io::Path>& search_paths)
{
   slang::TargetDesc target_desc{};
   target_desc.format = SLANG_SPIRV;
   target_desc.profile = m_global_session->findProfile(get_profile(format, target.stage));

   std::vector<slang::PreprocessorMacroDesc> preprocessor_macros;
   preprocessor_macros.reserve(definitions.size());
   for (const auto& [key, value] : definitions) {
      preprocessor_macros.emplace_back(slang::PreprocessorMacroDesc{key.data(), value.data()});
   }

   std::vector<const char*> search_paths_str;
   search_paths_str.reserve(search_paths.size());
   for (const auto& path : search_paths) {
      search_paths_str.emplace_back(path.string().data());
   }

   std::array<slang::CompilerOptionEntry, 2> compiler_options{
      slang::CompilerOptionEntry{.name = slang::CompilerOptionName::Optimization,
                                 .value =
                                    {
                                       .kind = slang::CompilerOptionValueKind::Int,
                                       .intValue0 = static_cast<i32>(build_type_to_optimization_level(m_build_type)),
                                    }},
      slang::CompilerOptionEntry{.name = slang::CompilerOptionName::DebugInformation,
                                 .value =
                                    {
                                       .kind = slang::CompilerOptionValueKind::Int,
                                       .intValue0 = static_cast<i32>(build_type_to_debug_information_level(m_build_type)),
                                    }},
   };

   slang::SessionDesc session_desc{};
   session_desc.targets = &target_desc;
   session_desc.targetCount = 1;
   session_desc.fileSystem = new FileSystem();
   session_desc.preprocessorMacroCount = static_cast<SlangInt>(preprocessor_macros.size());
   session_desc.preprocessorMacros = preprocessor_macros.data();
   session_desc.searchPathCount = static_cast<SlangInt>(search_paths.size());
   session_desc.searchPaths = search_paths_str.data();
   session_desc.compilerOptionEntryCount = compiler_options.size();
   session_desc.compilerOptionEntries = compiler_options.data();
   session_desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

   Slang::ComPtr<slang::ISession> session;
   m_global_session->createSession(session_desc, session.writeRef());

   Slang::ComPtr<slang::IModule> module;
   {
      Slang::ComPtr<slang::IBlob> diagnostic_blob;
      const auto basename = m_source_path.basename();
      module = session->loadModuleFromSourceString(basename.data(), m_source_path.string().data(), m_source_shader.data(),
                                                   diagnostic_blob.writeRef());
      if (!module) {
         print_diagnostic_blob(*diagnostic_blob);
         return std::nullopt;
      }
   }

   Slang::ComPtr<slang::IEntryPoint> entry_point;
   {
      Slang::ComPtr<slang::IBlob> diagnostic_blob;
      const auto* entry_point_name = get_entry_point(target);
      module->findAndCheckEntryPoint(entry_point_name, to_slang_stage(target.stage), entry_point.writeRef(), diagnostic_blob.writeRef());
      if (!entry_point) {
         print_diagnostic_blob(*diagnostic_blob);
         return std::nullopt;
      }
   }

   std::array<slang::IComponentType*, 2> component_types = {
      module,
      entry_point,
   };

   Slang::ComPtr<slang::IComponentType> composed_program;
   {
      Slang::ComPtr<slang::IBlob> diagnostics_blob;
      SlangResult result = session->createCompositeComponentType(component_types.data(), component_types.size(),
                                                                 composed_program.writeRef(), diagnostics_blob.writeRef());
      if (SLANG_FAILED(result)) {
         print_diagnostic_blob(*diagnostics_blob);
         return std::nullopt;
      }
   }

   Slang::ComPtr<slang::IComponentType> linked_program;
   {
      Slang::ComPtr<slang::IBlob> diagnostics_blob;
      SlangResult result = composed_program->link(linked_program.writeRef(), diagnostics_blob.writeRef());
      if (SLANG_FAILED(result)) {
         print_diagnostic_blob(*diagnostics_blob);
         return std::nullopt;
      }
   }

   slang::ProgramLayout* layout = linked_program->getLayout(0);
   this->process_reflection(*layout);

   Slang::ComPtr<slang::IBlob> target_code;
   {
      Slang::ComPtr<slang::IBlob> diagnostics_blob;
      SlangResult result = linked_program->getEntryPointCode(0,// entryPointIndex
                                                             0,// targetIndex
                                                             target_code.writeRef(), diagnostics_blob.writeRef());
      if (SLANG_FAILED(result)) {
         print_diagnostic_blob(*diagnostics_blob);
         return std::nullopt;
      }
   }

   std::vector<u8> blob_data(target_code->getBufferSize());
   std::memcpy(blob_data.data(), target_code->getBufferPointer(), target_code->getBufferSize());

   return std::make_optional<asset::ShaderBlob>(format, std::move(blob_data));
}

void Compiler::process_reflection(slang::ProgramLayout& layout)
{
   for (u32 i = 0; i < layout.getParameterCount(); ++i) {
      auto* param = layout.getParameterByIndex(i);

      auto& dst_param = m_properties[param->getName()];
      dst_param.name = make_name_id(param->getName());
      dst_param.vulkan_descriptor_binding = param->getBindingIndex();
      dst_param.vulkan_descriptor_set = param->getBindingSpace();
      dst_param.type = to_shader_property_type(param->getCategory());
   }
}

std::vector<asset::ShaderProperty> Compiler::get_properties() const
{
   std::vector<asset::ShaderProperty> result;
   for (const auto& prop : m_properties | std::views::values) {
      result.emplace_back(prop);
   }
   return result;
}

}// namespace triglav::shader_compiler
