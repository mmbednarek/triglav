#pragma once

#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <map>
#include <vector>

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::renderer::ui {

struct SpriteData
{
   graphics_api::Buffer vsUbo;
};

class SpriteRenderer
{
 public:
   using Self = SpriteRenderer;

   SpriteRenderer(graphics_api::Device& device, ui_core::Viewport& viewport, resource::ResourceManager& resourceManager);

   void on_added_sprite(ui_core::SpriteId id, const ui_core::Sprite& sprite);
   void on_updated_sprite(ui_core::SpriteId id, const ui_core::Sprite& sprite);
   void on_removed_sprite(ui_core::SpriteId id);

   void build_render_ui(render_core::BuildContext& ctx);

 private:
   [[nodiscard]] u32 get_texture_id(TextureName name);

   graphics_api::Device& m_device;
   ui_core::Viewport& m_viewport;
   resource::ResourceManager& m_resourceManager;
   std::vector<render_core::TextureRef> m_textures;
   std::map<TextureName, u32> m_textureIDs;
   std::map<ui_core::SpriteId, SpriteData> m_sprites;

   TG_SINK(ui_core::Viewport, OnAddedSprite);
   TG_SINK(ui_core::Viewport, OnUpdatedSprite);
   TG_SINK(ui_core::Viewport, OnRemovedSprite);
};

}// namespace triglav::renderer::ui
