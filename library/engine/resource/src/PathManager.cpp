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
      auto RA_contentPath = m_cachedContentPath.read_access();
      if (RA_contentPath->has_value()) {
         return **RA_contentPath;
      }
   }

   auto WA_contentPath = m_cachedContentPath.access();

   auto commandLineArg = io::CommandLine::the().arg("contentDir"_name);
   if (commandLineArg.has_value()) {
      WA_contentPath->emplace(io::Path{std::move(*commandLineArg)});
      return **WA_contentPath;
   }

   // TODO: Handle the situation when we don't get working directory
   auto workingDir = io::working_path().value();

   WA_contentPath->emplace(workingDir.sub("content"));
   return **WA_contentPath;
}

io::Path PathManager::build_path()
{
   {
      auto RA_buildPath = m_cachedBuildPath.read_access();
      if (RA_buildPath->has_value()) {
         return **RA_buildPath;
      }
   }

   auto WA_buildPath = m_cachedBuildPath.access();

   auto commandLineArg = io::CommandLine::the().arg("buildDir"_name);
   if (commandLineArg.has_value()) {
      WA_buildPath->emplace(io::Path{std::move(*commandLineArg)});
      return **WA_buildPath;
   }

   // TODO: Handle the situation when we don't get working directory
   auto workingDir = io::working_path().value();

   WA_buildPath->emplace(workingDir.parent().parent());
   return **WA_buildPath;
}

}// namespace triglav::resource