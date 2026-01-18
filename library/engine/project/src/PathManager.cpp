#include "PathManager.hpp"

#include "ProjectManager.hpp"

#include "triglav/BuildInfo.hpp"
#include "triglav/ResourcePathMap.hpp"
#include "triglav/io/CommandLine.hpp"

namespace triglav::project {

using namespace name_literals;

namespace {

std::string apply_constants(std::string input)
{
   static constexpr std::string_view build_profile_const = "{BUILD_PROFILE}";

   if (const auto pos = input.find(build_profile_const); pos != std::string::npos) {
      input.replace(pos, build_profile_const.size(), TG_STRING(TG_BUILD_PROFILE));
   }

   return input;
}

void copy_mapping(const Name proj_name, std::vector<PathMapping>& out_mappings)
{
   const auto* metadata = ProjectManager::the().project_metadata(proj_name);
   assert(metadata);
   for (const auto& src_mapping : metadata->resource_mapping) {
      const io::Path base_path(ProjectManager::the().project_root(proj_name));
      const auto sys_path = apply_constants(src_mapping.system_path);
      out_mappings.emplace_back(src_mapping.engine_path, base_path.sub(sys_path));
   }
}

}// namespace

PathManager& PathManager::the()
{
   static PathManager instance;
   return instance;
}

PathManager::PathManager()
{
   const auto dst_mapping = m_path_mappings.access();

   copy_mapping(engine_project(), dst_mapping.value());
   copy_mapping(this_project(), dst_mapping.value());

   if (this_project() != game_project()) {
      copy_mapping(game_project(), dst_mapping.value());
   }
}

io::Path PathManager::translate_path(const ResourceName rc_name) const
{
   const auto rc_string = ResourcePathMap::the().resolve(rc_name);

   const auto mappings = m_path_mappings.read_access();
   for (const auto& mapping : mappings.value()) {
      if (!rc_string.starts_with(StringView{mapping.input_path_prefix}))
         continue;

      return mapping.output_path_prefix.sub(rc_string.to_std().substr(mapping.input_path_prefix.length()));
   }

   assert(0);
   return {};
}

std::pair<io::Path, ResourceName> PathManager::import_path(const ResourceType rc_type, const std::string_view path) const
{
   const auto* project_md = ProjectManager::the().project_metadata(game_project());
   if (project_md == nullptr)
      return {};

   const auto sub_path = project_md->default_import_path(rc_type, path);
   const auto rc_name = name_from_path(sub_path);
   return {translate_path(name_from_path(sub_path)), rc_name};
}

}// namespace triglav::project