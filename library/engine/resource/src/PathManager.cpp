#include "PathManager.hpp"

#include "triglav/BuildInfo.hpp"
#include "triglav/io/CommandLine.hpp"
#include "triglav/project/Project.hpp"

namespace triglav::resource {

using namespace name_literals;

PathManager& PathManager::the()
{
   static PathManager instance;
   return instance;
}

const io::Path& PathManager::content_path()
{
   {
      auto RA_content_path = m_cached_content_path.read_access();
      if (RA_content_path->has_value()) {
         return **RA_content_path;
      }
   }

   auto WA_content_path = m_cached_content_path.access();

   auto command_line_arg = io::CommandLine::the().arg("contentDir"_name);
   if (command_line_arg.has_value()) {
      WA_content_path->emplace(*command_line_arg);
      return **WA_content_path;
   }

   const auto proj_info = project::load_current_project_info();
   if (proj_info.has_value()) {
      WA_content_path->emplace(io::Path{proj_info->path}.sub("content"));
      return **WA_content_path;
   }

   auto working_dir = io::working_path().value();
   WA_content_path->emplace(working_dir.sub("content"));
   return **WA_content_path;
}

std::optional<io::Path> PathManager::engine_content_path()
{
   {
      auto RA_content_path = m_cached_engine_content_path.read_access();
      if (RA_content_path->has_value()) {
         return **RA_content_path;
      }
   }

   auto WA_content_path = m_cached_engine_content_path.access();

   const auto proj_info = project::load_engine_project_info();
   if (proj_info.has_value()) {
      WA_content_path->emplace(io::Path{proj_info->path}.sub("content"));
      return **WA_content_path;
   }

   return std::nullopt;
}

std::optional<io::Path> PathManager::project_content_path()
{
   {
      auto RA_content_path = m_cached_project_content_path.read_access();
      if (RA_content_path->has_value()) {
         return **RA_content_path;
      }
   }

   auto WA_content_path = m_cached_project_content_path.access();

   auto project_arg = io::CommandLine::the().arg("project"_name);
   if (!project_arg.has_value()) {
      return std::nullopt;
   }

   const auto proj_info = project::load_project_info(*project_arg);
   if (proj_info.has_value()) {
      WA_content_path->emplace(io::Path{proj_info->path}.sub("content"));
      return **WA_content_path;
   }

   return std::nullopt;
}

const io::Path& PathManager::build_path()
{
   {
      auto RA_build_path = m_cached_build_path.read_access();
      if (RA_build_path->has_value()) {
         return **RA_build_path;
      }
   }

   auto WA_build_path = m_cached_build_path.access();

   auto command_line_arg = io::CommandLine::the().arg("buildDir"_name);
   if (command_line_arg.has_value()) {
      WA_build_path->emplace(io::Path{std::move(*command_line_arg)});
      return **WA_build_path;
   }

   const auto proj_info = project::load_engine_project_info();
   if (proj_info.has_value()) {
      WA_build_path->emplace(io::Path{proj_info->path}.sub("build").sub(TG_STRING(TG_BUILD_PROFILE)));
      return **WA_build_path;
   }

   auto working_dir = io::working_path().value();
   WA_build_path->emplace(working_dir.parent().parent());
   return **WA_build_path;
}

}// namespace triglav::resource