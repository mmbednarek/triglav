#include "PathManager.h"

#include "triglav/io/CommandLine.h"

namespace triglav::resource {

using namespace name_literals;

PathManager& PathManager::the()
{
   static PathManager instance;
   return instance;
}

io::Path PathManager::content_path()
{
   if (m_cachedContentPath.has_value()) {
      return m_cachedContentPath.value();
   }

   auto commandLineArg = io::CommandLine::the().arg("contentDir"_name);
   if (commandLineArg.has_value()) {
      m_cachedContentPath.emplace(io::Path{std::move(*commandLineArg)});
      return m_cachedContentPath.value();
   }

   // TODO: Handle the situation when we don't get working directory
   auto workingDir = io::working_path().value();


   m_cachedContentPath.emplace(workingDir.sub("content"));
   return m_cachedContentPath.value();
}

io::Path PathManager::build_path()
{
   if (m_cachedBuildPath.has_value()) {
      return m_cachedBuildPath.value();
   }

   auto commandLineArg = io::CommandLine::the().arg("buildDir"_name);
   if (commandLineArg.has_value()) {
      m_cachedBuildPath.emplace(io::Path{std::move(*commandLineArg)});
      return m_cachedBuildPath.value();
   }

   // TODO: Handle the situation when we don't get working directory
   auto workingDir = io::working_path().value();

   m_cachedBuildPath.emplace(workingDir.parent().parent());
   return m_cachedBuildPath.value();
}

}// namespace triglav::resource