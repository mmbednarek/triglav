#include "ResourceManager.h"

#include "MaterialLoader.h"
#include "ModelLoader.h"
#include "ShaderLoader.h"
#include "TextureLoader.h"
#include "TypefaceLoader.h"
#include "LevelLoader.h"
#include "StaticResources.h"

#include "triglav/io/File.h"
#include "triglav/TypeMacroList.hpp"

#include <ryml.hpp>
#include <spdlog/spdlog.h>

namespace triglav::resource {

namespace {

struct ResourcePath
{
   std::string name;
   std::string source;
};

std::vector<ResourcePath> parse_asset_list(const std::string_view path)
{
   std::vector<ResourcePath> result{};

   auto file      = io::read_whole_file(path);
   auto tree      = ryml::parse_in_place(c4::substr{const_cast<char *>(path.data()), path.size()},
                                         c4::substr{file.data(), file.size()});
   auto resources = tree["resources"];

   for (const auto node : resources) {
      auto name   = node["name"].val();
      auto source = node["source"].val();
      result.emplace_back(std::string{name.data(), name.size()}, std::string{source.data(), source.size()});
   }

   return result;
}

}// namespace

ResourceManager::ResourceManager(graphics_api::Device &device, font::FontManger &fontManager) :
    m_device(device),
    m_fontManager(fontManager)
{
#define TG_RESOURCE_TYPE(name, extension, cppType) \
   m_containers.emplace(ResourceType::name, std::make_unique<Container<ResourceType::name>>());
   TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE

   this->load_asset_list("../engine.yaml");

   register_samplers(device, *this);
}

void ResourceManager::load_asset_list(const std::string_view path)
{
   auto list = parse_asset_list(path);
   spdlog::info("Loading {} assets", list.size());

   int loadedCount{};
   for (const auto &[nameStr, source] : list) {
      auto name = make_name(nameStr);
      m_registeredNames.emplace(name, nameStr);
      spdlog::info("[{}/{}] {}", loadedCount, list.size(), nameStr);
      this->load_asset(name, source);
      ++loadedCount;
   }

   spdlog::info("[{}/{}] DONE", loadedCount, list.size());
}

void ResourceManager::load_asset(const Name assetName, const std::string_view path)
{
   switch (assetName.type()) {
#define TG_RESOURCE_TYPE(name, extension, cppType) \
   case ResourceType::name: this->load_resource<ResourceType::name>(assetName, path); break;
      TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
   case ResourceType::Unknown: break;
   }
}

bool ResourceManager::is_name_registered(const Name assetName) const
{
   if (not m_containers.contains(assetName.type()))
      return false;

   auto &container = m_containers.at(assetName.type());
   return container->is_name_registered(assetName);
}

}// namespace triglav::resource