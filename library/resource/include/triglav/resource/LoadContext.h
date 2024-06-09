#pragma once

#include "Resource.hpp"

#include "triglav/Int.hpp"
#include "triglav/io/Path.h"

#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

namespace triglav::resource {

struct ResourcePath
{
   std::string name;
   std::string source;
   ResourceProperties properties;
};

struct ResourceStage
{
   std::vector<ResourcePath> resourceList;
};

enum class FinishLoadingAssetResult
{
   None,
   FinishedStage,
   FinishedLoadingAssets,
};

class LoadContext
{
 public:
   explicit LoadContext(std::vector<ResourceStage>&& loadingStages);

   const ResourceStage& next_stage();
   FinishLoadingAssetResult finish_loading_asset();

   [[nodiscard]] u32 total_assets() const;
   [[nodiscard]] u32 total_loaded_assets() const;
   [[nodiscard]] u32 current_stage_id() const;

   static std::unique_ptr<LoadContext> from_asset_list(const io::Path& path);

 private:
   u32 m_assetsLoadedInStage{};
   u32 m_totalAssets{};
   u32 m_totalLoadedAssets{};
   u32 m_currentStageId{};
   std::vector<ResourceStage> m_loadingStages{};
   mutable std::shared_mutex m_mutex;
};

}// namespace triglav::resource