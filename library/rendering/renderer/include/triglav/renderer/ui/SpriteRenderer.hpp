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
   Vector4 spriteRect;
   Vector4 uvRect;
   Vector4 croppingMask;
   u32 imageID;
   u32 padding[3];
};

static_assert(sizeof(SpritePrimitive) % 16 == 0);

struct SpriteWriteData
{
   SpritePrimitive primitive;
   u32 dstIndex;
   u32 padding[3];
};

static_assert(sizeof(SpriteWriteData) % 16 == 0);

struct SpriteCopyInfo
{
   u32 srcID;
   u32 dstID;
};

class SpriteRenderer
{
 public:
   using Self = SpriteRenderer;

   SpriteRenderer(graphics_api::Device& device, ui_core::Viewport& viewport, resource::ResourceManager& resourceManager);

   void on_added_sprite(ui_core::SpriteId id, const ui_core::Sprite& sprite);
   void on_updated_sprite(ui_core::SpriteId id, const ui_core::Sprite& sprite);
   void on_removed_sprite(ui_core::SpriteId id);

   void set_object(u32 index, const SpritePrimitive& prim);
   void move_object(u32 src, u32 dst);

   void prepare_frame(render_core::JobGraph& graph, u32 frameIndex);
   void build_data_update(render_core::BuildContext& ctx) const;
   void build_render_ui(render_core::BuildContext& ctx);

 private:
   [[nodiscard]] u32 get_texture_id(TextureName name);

   graphics_api::Device& m_device;
   ui_core::Viewport& m_viewport;
   resource::ResourceManager& m_resourceManager;
   std::vector<render_core::TextureRef> m_textures;
   std::map<TextureName, u32> m_textureIDs;
   std::array<UpdateList<ui_core::RectId, SpritePrimitive>, render_core::FRAMES_IN_FLIGHT_COUNT> m_frameUpdates;

   SpriteWriteData* m_stagingInsertions;
   u32 m_stagingInsertionsTop{};
   SpriteCopyInfo* m_stagingRemovals;
   u32 m_stagingRemovalsTop{};

   TG_SINK(ui_core::Viewport, OnAddedSprite);
   TG_SINK(ui_core::Viewport, OnUpdatedSprite);
   TG_SINK(ui_core::Viewport, OnRemovedSprite);
};

}// namespace triglav::renderer::ui
