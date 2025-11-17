#pragma once

#include "triglav/UpdateList.hpp"
#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <map>
#include <vector>

namespace triglav::render_core {
class JobGraph;
class BuildContext;
}// namespace triglav::render_core

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::renderer::ui {

struct SpritePrimitive
{
   Vector4 sprite_rect;
   Vector4 uv_rect;
   Vector4 cropping_mask;
   u32 image_id;
   u32 padding[3];
};

static_assert(sizeof(SpritePrimitive) % 16 == 0);

struct SpriteWriteData
{
   SpritePrimitive primitive;
   u32 dst_index;
   u32 padding[3];
};

static_assert(sizeof(SpriteWriteData) % 16 == 0);

struct SpriteCopyInfo
{
   u32 src_id;
   u32 dst_id;
};

class SpriteRenderer
{
 public:
   using Self = SpriteRenderer;

   SpriteRenderer(ui_core::Viewport& viewport, resource::ResourceManager& resource_manager);

   void on_added_sprite(ui_core::SpriteId id, const ui_core::Sprite& sprite);
   void on_updated_sprite(ui_core::SpriteId id, const ui_core::Sprite& sprite);
   void on_removed_sprite(ui_core::SpriteId id);

   void set_object(u32 index, const SpritePrimitive& prim);
   void move_object(u32 src, u32 dst);

   void prepare_frame(render_core::JobGraph& graph, u32 frame_index);
   void build_data_update(render_core::BuildContext& ctx) const;
   void build_render_ui(render_core::BuildContext& ctx);

 private:
   [[nodiscard]] u32 get_texture_id(TextureName name);

   resource::ResourceManager& m_resource_manager;
   std::vector<render_core::TextureRef> m_textures;
   std::map<TextureName, u32> m_texture_ids;
   std::array<UpdateList<ui_core::RectId, SpritePrimitive>, render_core::FRAMES_IN_FLIGHT_COUNT> m_frame_updates;

   SpriteWriteData* m_staging_insertions;
   u32 m_staging_insertions_top{};
   SpriteCopyInfo* m_staging_removals;
   u32 m_staging_removals_top{};

   TG_SINK(ui_core::Viewport, OnAddedSprite);
   TG_SINK(ui_core::Viewport, OnUpdatedSprite);
   TG_SINK(ui_core::Viewport, OnRemovedSprite);
};

}// namespace triglav::renderer::ui
