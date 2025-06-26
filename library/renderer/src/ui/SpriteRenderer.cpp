#include "ui/SpriteRenderer.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::renderer::ui {

using namespace name_literals;

using graphics_api::BufferUsage;

struct SpriteUBO
{
   Vector2 spriteOffset;
   Vector2 spriteSize;
   Vector2 uvOffset;
   Vector2 uvSize;
   Vector2 screenSize;
   u32 imageID;
};

namespace {

SpriteUBO to_sprite_ubo(const ui_core::Sprite& sprite, const u32 imageID, const Vector2 screenSize, const Vector2 textureSize)
{
   Vector2 uvOffset{0, 0};
   Vector2 uvSize{1, 1};

   if (sprite.textureRegion.has_value()) {
      uvOffset = Vector2(sprite.textureRegion->x, sprite.textureRegion->y) / textureSize;
      uvSize = Vector2(sprite.textureRegion->z, sprite.textureRegion->w) / textureSize;
   }

   return {
      .spriteOffset = sprite.position,
      .spriteSize = sprite.size,
      .uvOffset = uvOffset,
      .uvSize = uvSize,
      .screenSize = screenSize,
      .imageID = imageID,
   };
}

}// namespace

SpriteRenderer::SpriteRenderer(graphics_api::Device& device, ui_core::Viewport& viewport, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_viewport(viewport),
    m_resourceManager(resourceManager),
    TG_CONNECT(viewport, OnAddedSprite, on_added_sprite),
    TG_CONNECT(viewport, OnSpriteChangePosition, on_sprite_change_position),
    TG_CONNECT(viewport, OnSpriteChangeTextureRegion, on_sprite_change_texture_region),
    TG_CONNECT(viewport, OnRemovedSprite, on_removed_sprite)
{
}

void SpriteRenderer::on_added_sprite(const Name id, const ui_core::Sprite& sprite)
{
   auto buffer = GAPI_CHECK(m_device.create_buffer(BufferUsage::UniformBuffer | BufferUsage::TransferDst, sizeof(SpriteUBO)));

   const auto texSizeRes = m_resourceManager.get(sprite.texture).resolution();
   const Vector2 texSize{texSizeRes.width, texSizeRes.height};

   const auto uboData = to_sprite_ubo(sprite, this->get_texture_id(sprite.texture), m_viewport.dimensions(), texSize);
   GAPI_CHECK_STATUS(buffer.write_indirect(&uboData, sizeof(SpriteUBO)));

   m_sprites.emplace(id, SpriteData{std::move(buffer)});
}

void SpriteRenderer::on_sprite_change_position(const Name id, const ui_core::Sprite& sprite)
{
   const auto texSizeRes = m_resourceManager.get(sprite.texture).resolution();
   const Vector2 texSize{texSizeRes.width, texSizeRes.height};
   const auto uboData = to_sprite_ubo(sprite, this->get_texture_id(sprite.texture), m_viewport.dimensions(), texSize);
   GAPI_CHECK_STATUS(m_sprites.at(id).vsUbo.write_indirect(&uboData, sizeof(SpriteUBO)));
}

void SpriteRenderer::on_sprite_change_texture_region(const Name id, const ui_core::Sprite& sprite)
{
   this->on_sprite_change_position(id, sprite);
}

void SpriteRenderer::on_removed_sprite(const Name id)
{
   m_sprites.erase(id);
}

void SpriteRenderer::build_render_ui(render_core::BuildContext& ctx)
{
   for (const auto& sprite : Values(m_sprites)) {
      ctx.bind_vertex_shader("sprite.vshader"_rc);

      ctx.bind_uniform_buffer(0, &sprite.vsUbo);

      ctx.bind_fragment_shader("sprite.fshader"_rc);

      ctx.bind_sampled_texture_array(1, m_textures);
      ctx.set_vertex_topology(graphics_api::VertexTopology::TriangleFan);

      ctx.draw_primitives(4, 0);
   }
}

u32 SpriteRenderer::get_texture_id(const TextureName name)
{
   const auto it = m_textureIDs.find(name);
   if (it != m_textureIDs.end()) {
      return it->second;
   }

   const auto id = static_cast<u32>(m_textures.size());
   m_textures.emplace_back(name);
   m_textureIDs.emplace(name, id);
   return id;
}

}// namespace triglav::renderer::ui
