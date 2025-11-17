#include "widget/Image.hpp"

#include "Context.hpp"
#include "Viewport.hpp"

#include "triglav/graphics_api/Texture.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::ui_core {

namespace {

Vector2 calculate_proportional_size(const Vector2 parent_size, const Vector2 image_size)
{
   const float parent_prop = parent_size.y / parent_size.x;
   const float tex_prop = image_size.y / image_size.x;

   if (tex_prop > parent_prop) {
      return {image_size.x * (parent_size.y / image_size.y), parent_size.y};
   }

   return {parent_size.x, image_size.y * (parent_size.x / image_size.x)};
}

}// namespace

Image::Image(Context& ctx, const State state, IWidget* /*parent*/) :
    m_context(ctx),
    m_state(state)
{
}

Vector2 Image::desired_size(const Vector2 parent_size) const
{
   const auto& tex = m_context.resource_manager().get(m_state.texture);
   Vector2 tex_size{tex.width(), tex.height()};

   if (m_state.region.has_value()) {
      tex_size = {m_state.region->z, m_state.region->w};
   }

   const auto prop_size = calculate_proportional_size(parent_size, tex_size);
   if (!m_state.max_size.has_value() || (m_state.max_size->x >= prop_size.x && m_state.max_size->y >= prop_size.y)) {
      return prop_size;
   }

   return calculate_proportional_size(*m_state.max_size, tex_size);
}

void Image::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   if (!do_regions_intersect(dimensions, cropping_mask)) {
      if (m_sprite_id != 0) {
         m_context.viewport().remove_sprite(m_sprite_id);
         m_sprite_id = 0;
      }
      return;
   }

   if (m_sprite_id != 0) {
      m_context.viewport().set_sprite_position(m_sprite_id, {dimensions.x, dimensions.y}, cropping_mask);
      return;
   }

   m_sprite_id = m_context.viewport().add_sprite(Sprite{
      .texture = m_state.texture,
      .position = {dimensions.x, dimensions.y},
      .size = {dimensions.z, dimensions.w},
      .crop = cropping_mask,
      .texture_region = m_state.region,
   });
}

void Image::remove_from_viewport()
{
   m_context.viewport().remove_sprite(m_sprite_id);
   m_sprite_id = 0;
}

void Image::set_region(const Vector4 region)
{
   m_state.region = region;
   m_context.viewport().set_sprite_texture_region(m_sprite_id, region);
}

}// namespace triglav::ui_core
