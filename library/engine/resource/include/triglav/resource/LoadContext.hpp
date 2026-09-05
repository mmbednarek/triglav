#pragma once

#include "triglav/Int.hpp"
#include "triglav/Name.hpp"
#include "triglav/String.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/threading/SharedMutex.hpp"

#include <memory>
#include <set>
#include <vector>

namespace triglav::resource {

struct ResourceStage
{
   std::vector<std::pair<ResourceName, io::Path>> resource_list;
};

using LoadIndex = u32;

enum class FinishLoadingAssetResult
{
   None,
   FinishedStage,
   FinishedLoadingAssets,
};

class LoadContext
{
 public:
   explicit LoadContext(std::vector<ResourceStage>&& loading_stages, LoadIndex load_id);

   const ResourceStage& next_stage();
   FinishLoadingAssetResult finish_loading_asset();

   [[nodiscard]] u32 total_assets() const;
   [[nodiscard]] u32 total_loaded_assets() const;
   [[nodiscard]] u32 current_stage_id() const;
   [[nodiscard]] LoadIndex load_index() const;

   static std::unique_ptr<LoadContext> from_asset_list(const io::Path& path, LoadIndex load_id);
   static std::unique_ptr<LoadContext> from_target_asset(ResourceName res_name, LoadIndex load_id);
   static std::unique_ptr<LoadContext> from_assets(std::span<ResourceName> resources, LoadIndex load_id);

 private:
   static std::unique_ptr<LoadContext> build_load_context(const std::set<ResourceName>& resources, LoadIndex load_id);


   u32 m_assets_loaded_in_stage{};
   u32 m_total_assets{};
   u32 m_total_loaded_assets{};
   u32 m_current_stage_id{};
   LoadIndex m_load_id{};
   std::vector<ResourceStage> m_loading_stages{};
   mutable threading::SharedMutex m_mutex;
};

}// namespace triglav::resource