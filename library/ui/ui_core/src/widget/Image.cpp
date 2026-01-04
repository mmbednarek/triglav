#include "widget/Image.hpp"

#include "Context.hpp"
#include "Viewport.hpp"

#include "triglav/graphics_api/Texture.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::ui_core {

namespace {

Vector2 calculate_proportional_size(const Vector2 available_size, const Vector2 image_size)
{
   const float parent_prop = available_size.y / available_size.x;
   const float tex_prop = image_size.y / image_size.x;

   if (tex_prop > parent_prop) {
      return {image_size.x * (available_size.y / image_size.y), available_size.y};
   }

   return {available_size.x, image_size.y * (available_size.x / image_size.x)};
}

}// namespace

Image::Image(Context& ctx, const State state, IWidget* /*parent*/) :
    m_context(ctx),
    m_state(state),
    m_sprite{
       .texture = m_state.texture,
       .texture_region = m_state.region,
    }
{
}

Vector2 Image::desired_size(const Vector2 available_size) const
{
   const auto& tex = m_context.resource_manager().get(m_state.texture);
   Vector2 tex_size{tex.width(), tex.height()};

   if (m_state.region.has_value()) {
      tex_size = {m_state.region->z, m_state.region->w};
   }

   const auto prop_size = calculate_proportional_size(available_size, tex_size);
   if (!m_state.max_size.has_value() || (m_state.max_size->x >= prop_size.x && m_state.max_size->y >= prop_size.y)) {
      return prop_size;
   }

   return calculate_proportional_size(*m_state.max_size, tex_size);
}

void Image::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   if (!do_regions_intersect(dimensions, cropping_mask)) {
      m_sprite.remove(m_context);
      return;
   }

   m_sprite.add(m_context, dimensions, cropping_mask);
}

void Image::remove_from_viewport()
{
   m_sprite.remove(m_context);
}

void Image::set_region(const Vector4 region)
{
   m_state.region = region;
   m_sprite.set_region(m_context, region);
}

}// namespace triglav::ui_core
