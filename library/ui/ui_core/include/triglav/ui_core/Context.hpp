#pragma once

#include "triglav/Math.hpp"

namespace triglav::render_core {
class GlyphCache;
}

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::ui_core {

class Viewport;
class IWidget;

class Context
{
 public:
   Context(Viewport& viewport, render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager);

   [[nodiscard]] Viewport& viewport() const;
   [[nodiscard]] render_core::GlyphCache& glyph_cache() const;
   [[nodiscard]] resource::ResourceManager& resource_manager() const;

   void set_active_widget(ui_core::IWidget* active_widget, Vector4 active_area);
   [[nodiscard]] ui_core::IWidget* active_widget() const;
   [[nodiscard]] Vector4 active_area() const;

 private:
   Viewport& m_viewport;
   render_core::GlyphCache& m_glyph_cache;
   resource::ResourceManager& m_resource_manager;

   IWidget* m_active_widget = nullptr;
   Vector4 m_active_area{};
};

}// namespace triglav::ui_core
