#include "ProjectManager.hpp"

#include "Name.hpp"

#include "triglav/io/CommandLine.hpp"
#include "triglav/io/File.hpp"
#include "triglav/json_util/Deserialize.hpp"
#include "triglav/json_util/Serialize.hpp"

#include <algorithm>
#include <ranges>

namespace triglav::project {

using namespace name_literals;

namespace {


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

std::optional<io::Path> get_project_config_path()
{
   const auto dir = io::home_directory();
   if (!dir.has_value()) {
      return std::nullopt;
   }

   const auto triglav_dir = dir->sub(".triglav");
   if (!triglav_dir.exists()) {
      if (!io::make_directory(triglav_dir))
         return std::nullopt;
   }

   return triglav_dir.sub("projects.json");
}

ProjectConfig load_project_config()
{
   const auto project_config_path = get_project_config_path();
   if (!project_config_path.has_value())
      return {};

   const auto project_file = io::open_file(*project_config_path, io::FileMode::Read);
   if (!project_file.has_value())
      return {};

   ProjectConfig config;
   if (!json_util::deserialize(config.to_meta_ref(), **project_file))
      return {};

   return config;
}

}// namespace

ProjectManager::ProjectManager() :
    m_config(load_project_config())
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

Name ProjectManager::active_project() const
{
   return make_name_id(m_config.active_project);
}

std::string_view ProjectManager::active_project_identifier() const
{
   return m_config.active_project;
}

const std::vector<ProjectInfo>& ProjectManager::projects() const
{
   return m_config.projects;
}

const ProjectInfo* ProjectManager::active_project_info() const
{
   return this->project_info(this->active_project());
}

const ProjectMetadata* ProjectManager::active_project_metadata()
{
   return this->project_metadata(this->active_project());
}

void ProjectManager::add_project(const std::string_view project_name, const io::Path& project_path)
{
   const auto it = std::ranges::find_if(m_config.projects, [&](const ProjectInfo& info) { return info.name == project_name; });
   if (it != m_config.projects.end()) {
      return;// already exists
   }
   m_config.projects.emplace_back(std::string(project_name), std::string(project_path.string()), make_name_id(project_name));
}

bool ProjectManager::set_active_project(const std::string_view project_name)
{
   const auto it = std::ranges::find_if(m_config.projects, [&](const ProjectInfo& info) { return info.name == project_name; });
   if (it == m_config.projects.end()) {
      return false;// doesn't exists
   }
   m_config.active_project = project_name;
   return true;
}

bool ProjectManager::save_project_info()
{
   const auto project_config_path = get_project_config_path();
   if (!project_config_path.has_value())
      return false;

   const auto file = io::open_file(*project_config_path, io::FileMode::Write | io::FileMode::Create);
   if (!file.has_value())
      return false;

   json_util::serialize(m_config.to_meta_ref(), **file, true);
   return true;
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

   if (project_name() != "triglav_editor" && project_name() != "triglav_cli") {
      cached = this_project();
      return this_project();
   }

   const auto arg = io::CommandLine::the().arg("project"_name);
   if (arg.has_value()) {
      cached = make_name_id(arg.value());
      return *cached;
   }

   const auto active_project_info = ProjectManager::the().active_project_info();
   if (active_project_info != nullptr) {
      cached = active_project_info->name_id;
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
