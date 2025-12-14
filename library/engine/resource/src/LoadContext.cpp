#include "LoadContext.hpp"

#include "triglav/ResourcePathMap.hpp"
#include "triglav/io/File.hpp"

#include <ryml.hpp>

#include <mutex>
#include <shared_mutex>

namespace triglav::resource {

LoadContext::LoadContext(std::vector<ResourceStage>&& loading_stages) :
    m_loading_stages(std::move(loading_stages))
{
   for (const auto& stage : m_loading_stages) {
      m_total_assets += static_cast<u32>(stage.resource_list.size());
   }
}

const ResourceStage& LoadContext::next_stage()
{
   std::unique_lock lk{m_mutex};
   m_assets_loaded_in_stage = 0;
   return m_loading_stages[m_current_stage_id];
}

FinishLoadingAssetResult LoadContext::finish_loading_asset()
{
   std::unique_lock lk{m_mutex};

   ++m_assets_loaded_in_stage;
   ++m_total_loaded_assets;

   if (m_assets_loaded_in_stage >= m_loading_stages[m_current_stage_id].resource_list.size()) {
      ++m_current_stage_id;
      if (m_current_stage_id >= m_loading_stages.size()) {
         return FinishLoadingAssetResult::FinishedLoadingAssets;
      }

      return FinishLoadingAssetResult::FinishedStage;
   }

   return FinishLoadingAssetResult::None;
}

u32 LoadContext::total_assets() const
{
   std::shared_lock lk{m_mutex};
   return m_total_assets;
}
u32 LoadContext::total_loaded_assets() const
{
   std::shared_lock lk{m_mutex};
   return m_total_loaded_assets;
}
u32 LoadContext::current_stage_id() const
{
   std::shared_lock lk{m_mutex};
   return m_current_stage_id;
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
      auto rc_path = node.val();
      auto resource_name = name_from_path(StringView{rc_path.data(), rc_path.size()});
      result[resource_name.loading_stage()].resource_list.emplace_back(rc_path.data(), rc_path.size());
   }

   result.erase(std::ranges::remove_if(result, [](const ResourceStage& stage) { return stage.resource_list.empty(); }).begin(),
                result.end());

   return std::make_unique<LoadContext>(std::move(result));
}

}// namespace triglav::resource