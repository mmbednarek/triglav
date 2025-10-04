#include "widget/Image.hpp"

#include "Context.hpp"
#include "Viewport.hpp"

#include "triglav/graphics_api/Texture.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::ui_core {

namespace {

Vector2 calculate_proportional_size(const Vector2 parentSize, const Vector2 imageSize)
{
   const float parentProp = parentSize.y / parentSize.x;
   const float texProp = imageSize.y / imageSize.x;

   if (texProp > parentProp) {
      return {imageSize.x * (parentSize.y / imageSize.y), parentSize.y};
   }

   return {parentSize.x, imageSize.y * (parentSize.x / imageSize.x)};
}

}// namespace

Image::Image(Context& ctx, const State state, IWidget* /*parent*/) :
    m_context(ctx),
    m_state(state)
{
}

Vector2 Image::desired_size(const Vector2 parentSize) const
{
   const auto& tex = m_context.resource_manager().get(m_state.texture);
   Vector2 texSize{tex.width(), tex.height()};

   if (m_state.region.has_value()) {
      texSize = {m_state.region->z, m_state.region->w};
   }

   const auto propSize = calculate_proportional_size(parentSize, texSize);
   if (!m_state.maxSize.has_value() || (m_state.maxSize->x >= propSize.x && m_state.maxSize->y >= propSize.y)) {
      return propSize;
   }

   return calculate_proportional_size(*m_state.maxSize, texSize);
}

void Image::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   if (!do_regions_intersect(dimensions, croppingMask)) {
      if (m_spriteID != 0) {
         m_context.viewport().remove_sprite(m_spriteID);
         m_spriteID = 0;
      }
      return;
   }

   if (m_spriteID != 0) {
      m_context.viewport().set_sprite_position(m_spriteID, {dimensions.x, dimensions.y});
      // TODO: Set sprite cropping!
      return;
   }

   m_spriteID = m_context.viewport().add_sprite(Sprite{
      .texture = m_state.texture,
      .position = {dimensions.x, dimensions.y},
      .size = {dimensions.z, dimensions.w},
      .crop = croppingMask,
      .textureRegion = m_state.region,
   });
}

void Image::remove_from_viewport()
{
   m_context.viewport().remove_sprite(m_spriteID);
   m_spriteID = 0;
}

void Image::set_region(const Vector4 region)
{
   m_state.region = region;
   m_context.viewport().set_sprite_texture_region(m_spriteID, region);
}

}// namespace triglav::ui_core
