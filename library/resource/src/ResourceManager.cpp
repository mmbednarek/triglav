#include "ResourceManager.h"

#include "GlyphAtlasLoader.h"
#include "LevelLoader.h"
#include "MaterialLoader.h"
#include "ModelLoader.h"
#include "PathManager.h"
#include "ShaderLoader.h"
#include "StaticResources.h"
#include "TextureLoader.h"
#include "TypefaceLoader.h"

#include "triglav/TypeMacroList.hpp"
#include "triglav/io/File.h"

#include <ryml.hpp>
#include <spdlog/spdlog.h>

namespace triglav::resource {

namespace {

struct ResourcePath
{
   std::string name;
   std::string source;
};

std::vector<ResourcePath> parse_asset_list(const io::Path& path)
{
   std::vector<ResourcePath> result{};

   auto file = io::read_whole_file(path);
   auto tree =
      ryml::parse_in_place(c4::substr{const_cast<char*>(path.string().data()), path.string().size()}, c4::substr{file.data(), file.size()});
   auto resources = tree["resources"];

   for (const auto node : resources) {
      auto name = node["name"].val();
      auto source = node["source"].val();
      result.emplace_back(std::string{name.data(), name.size()}, std::string{source.data(), source.size()});
   }

   return result;
}

}// namespace

ResourceManager::ResourceManager(graphics_api::Device& device, font::FontManger& fontManager) :
    m_device(device),
    m_fontManager(fontManager)
{
#define TG_RESOURCE_TYPE(name, extension, cppType) \
   m_containers.emplace(ResourceType::name, std::make_unique<Container<ResourceType::name>>());
   TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE

   this->load_asset_list(PathManager::the().content_path().sub("index.yaml"));

   register_samplers(device, *this);
}

void ResourceManager::load_asset_list(const io::Path& path)
{
   auto list = parse_asset_list(path);
   spdlog::info("Loading {} assets", list.size());

   auto buildPath = PathManager::the().build_path();
   auto contentPath = PathManager::the().content_path();

   int loadedCount{};
   for (const auto& [nameStr, source] : list) {
      auto resourcePath = buildPath.sub(source);
      if (not resourcePath.exists()) {
         resourcePath = contentPath.sub(source);

         if (not resourcePath.exists()) {
            spdlog::error("failed to load resource: {}, file not found", source);
            continue;
         }
      }

      auto name = make_rc_name(nameStr);
      m_registeredNames.emplace(name, nameStr);
      spdlog::info("[{}/{}] {}", loadedCount, list.size(), nameStr);
      this->load_asset(name, resourcePath);
      ++loadedCount;
   }

   spdlog::info("[{}/{}] DONE", loadedCount, list.size());
}

void ResourceManager::load_asset(const ResourceName assetName, const io::Path& path)
{
   switch (assetName.type()) {
#define TG_RESOURCE_TYPE(name, extension, cppType)              \
   case ResourceType::name:                                     \
      this->load_resource<ResourceType::name>(assetName, path); \
      break;
      TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
   case ResourceType::Unknown:
      break;
   }
}

bool ResourceManager::is_name_registered(const ResourceName assetName) const
{
   if (not m_containers.contains(assetName.type()))
      return false;

   auto& container = m_containers.at(assetName.type());
   return container->is_name_registered(assetName);
}

}// namespace triglav::resource