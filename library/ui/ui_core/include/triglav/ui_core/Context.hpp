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
   Context(Viewport& viewport, render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager);

   [[nodiscard]] Viewport& viewport() const;
   [[nodiscard]] render_core::GlyphCache& glyph_cache() const;
   [[nodiscard]] resource::ResourceManager& resource_manager() const;

   void set_active_widget(ui_core::IWidget* active_widget, Vector4 active_area);
   [[nodiscard]] ui_core::IWidget* active_widget() const;
   [[nodiscard]] Vector4 active_area() const;

 private:
   Viewport& m_viewport;
   render_core::GlyphCache& m_glyphCache;
   resource::ResourceManager& m_resourceManager;

   IWidget* m_activeWidget = nullptr;
   Vector4 m_activeArea{};
};

}// namespace triglav::ui_core
