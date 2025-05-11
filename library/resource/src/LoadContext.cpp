#include "LoadContext.hpp"

#include "triglav/io/File.hpp"

#include <ryml.hpp>

#include <mutex>
#include <shared_mutex>

namespace triglav::resource {

LoadContext::LoadContext(std::vector<ResourceStage>&& loadingStages) :
    m_loadingStages(std::move(loadingStages))
{
   for (const auto& stage : m_loadingStages) {
      m_totalAssets += stage.resourceList.size();
   }
}

const ResourceStage& LoadContext::next_stage()
{
   std::unique_lock lk{m_mutex};
   m_assetsLoadedInStage = 0;
   return m_loadingStages[m_currentStageId];
}

FinishLoadingAssetResult LoadContext::finish_loading_asset()
{
   std::unique_lock lk{m_mutex};

   ++m_assetsLoadedInStage;
   ++m_totalLoadedAssets;

   if (m_assetsLoadedInStage >= m_loadingStages[m_currentStageId].resourceList.size()) {
      ++m_currentStageId;
      if (m_currentStageId >= m_loadingStages.size()) {
         return FinishLoadingAssetResult::FinishedLoadingAssets;
      }

      return FinishLoadingAssetResult::FinishedStage;
   }

   return FinishLoadingAssetResult::None;
}

u32 LoadContext::total_assets() const
{
   std::shared_lock lk{m_mutex};
   return m_totalAssets;
}
u32 LoadContext::total_loaded_assets() const
{
   std::shared_lock lk{m_mutex};
   return m_totalLoadedAssets;
}
u32 LoadContext::current_stage_id() const
{
   std::shared_lock lk{m_mutex};
   return m_currentStageId;
}

std::unique_ptr<LoadContext> LoadContext::from_asset_list(const io::Path& path)
{
   std::vector<ResourceStage> result{};
   result.resize(loading_stage_count());

   auto file = io::read_whole_file(path);
   if (file.empty()) {
      return {};
   }

   auto tree =
      ryml::parse_in_place(c4::substr{const_cast<char*>(path.string().data()), path.string().size()}, c4::substr{file.data(), file.size()});
   auto resources = tree["resources"];

   for (const auto node : resources) {
      auto name = node["name"].val();
      auto source = node["source"].val();

      ResourceProperties properties;

      if (node.has_child("properties")) {
         auto propertiesNode = node["properties"];
         for (const auto property : propertiesNode) {
            auto key = property.key();
            auto value = property.val();
            properties.add(make_name_id({key.data(), key.size()}), std::string{value.data(), value.size()});
         }
      }

      auto resourceName = make_rc_name(std::string{name.data(), name.size()});
      result[resourceName.loading_stage()].resourceList.emplace_back(std::string{name.data(), name.size()},
                                                                     std::string{source.data(), source.size()}, std::move(properties));
   }

   result.erase(std::ranges::remove_if(result, [](const ResourceStage& stage) { return stage.resourceList.empty(); }).begin(),
                result.end());

   return std::make_unique<LoadContext>(std::move(result));
}

}// namespace triglav::resource