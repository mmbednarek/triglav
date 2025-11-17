#include "PathManager.hpp"

#include "triglav/io/CommandLine.hpp"

namespace triglav::resource {

using namespace name_literals;

PathManager& PathManager::the()
{
   static PathManager instance;
   return instance;
}

io::Path PathManager::content_path()
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
      WA_content_path->emplace(io::Path{std::move(*command_line_arg)});
      return **WA_content_path;
   }

   // TODO: Handle the situation when we don't get working directory
   auto working_dir = io::working_path().value();

   WA_content_path->emplace(working_dir.sub("content"));
   return **WA_content_path;
}

io::Path PathManager::build_path()
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

   // TODO: Handle the situation when we don't get working directory
   auto working_dir = io::working_path().value();

   WA_build_path->emplace(working_dir.parent().parent());
   return **WA_build_path;
}

}// namespace triglav::resource