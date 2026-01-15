#include "ProjectManager.hpp"

#include "Name.hpp"

#include "triglav/io/CommandLine.hpp"
#include "triglav/io/File.hpp"
#include "triglav/json_util/Deserialize.hpp"

#include <algorithm>
#include <ranges>

namespace triglav::project {

using namespace name_literals;

template<typename TIt, typename TValue, typename TTrans>
TIt find_binary(TIt begin, TIt end, TValue value, TTrans trans)
{
   const auto saved_end = end;
   while (begin != end) {
      auto mid = begin + std::distance(begin, end) / 2;
      const auto comp = trans(*mid);
      if (comp < value) {
         end = mid;
      } else if (comp > value) {
         begin = mid + 1;
      } else {
         return mid;
      }
   }
   return saved_end;
}

ProjectManager::ProjectManager() :
    m_config(load_project_config().value_or({}))
{
   for (auto& project_info : m_config.projects) {
      project_info.name_id = make_name_id(project_info.name);
   }
   // sort so we can binary search them
   std::ranges::sort(m_config.projects, [](const ProjectInfo& left, const ProjectInfo& right) { return left.name_id > right.name_id; });
}

const ProjectInfo* ProjectManager::project_info(const Name project_name) const
{
   const auto res =
      find_binary(m_config.projects.begin(), m_config.projects.end(), project_name, [](const ProjectInfo& left) { return left.name_id; });
   if (res == m_config.projects.end()) {
      return nullptr;
   }
   return &(*res);
}

std::string_view ProjectManager::project_root(const Name project_name) const
{
   const auto* info = this->project_info(project_name);
   if (info == nullptr)
      return {};
   return info->path;
}

const ProjectMetadata* ProjectManager::project_metadata(const Name project_name)
{
   if (m_metadata.contains(project_name))
      return &m_metadata.at(project_name);

   const auto project_json_path = io::Path{this->project_root(project_name)}.sub("project.json");
   const auto project_file = io::open_file(project_json_path, io::FileMode::Read);
   if (!project_file.has_value())
      return nullptr;

   ProjectMetadata result;
   if (!json_util::deserialize(result.to_meta_ref(), **project_file))
      return nullptr;

   auto [md, ok] = m_metadata.emplace(project_name, std::move(result));
   assert(ok);
   return &md->second;
}

ProjectManager& ProjectManager::the()
{
   static ProjectManager instance;
   return instance;
}

Name this_project()
{
   return make_name_id(project_name());
}

Name game_project()
{
   static std::optional<Name> cached;
   if (cached.has_value())
      return *cached;

   if (project_name() != "triglav_editor") {
      cached = this_project();
      return this_project();
   }

   const auto arg = io::CommandLine::the().arg("project"_name);
   if (arg.has_value()) {
      cached = make_name_id(arg.value());
      return *cached;
   }

   return this_project();
}

Name engine_project()
{
   const auto* proj_info = ProjectManager::the().project_metadata(this_project());
   if (proj_info == nullptr)
      return make_name_id("triglav");
   return make_name_id(proj_info->engine);
}

}// namespace triglav::project
