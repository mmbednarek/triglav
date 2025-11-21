#pragma once

#include "triglav/Logging.hpp"
#include "triglav/Math.hpp"

#include <vector>

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
   TG_DEFINE_LOG_CATEGORY(UI_Context)
 public:
   Context(Viewport& viewport, render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager);

   [[nodiscard]] Viewport& viewport() const;
   [[nodiscard]] render_core::GlyphCache& glyph_cache() const;
   [[nodiscard]] resource::ResourceManager& resource_manager() const;

   void set_active_widget(IWidget* active_widget, Vector4 active_area);
   [[nodiscard]] IWidget* active_widget() const;
   [[nodiscard]] Vector4 active_area() const;
   void add_activating_widget(IWidget* widget);
   void remove_activating_widget(IWidget* widget);

   void toggle_active_widget() const;

 private:
   Viewport& m_viewport;
   render_core::GlyphCache& m_glyph_cache;
   resource::ResourceManager& m_resource_manager;

   IWidget* m_active_widget = nullptr;
   Vector4 m_active_area{};

   std::vector<IWidget*> m_activating_widgets;
   MemorySize m_active_widget_id{};
};

}// namespace triglav::ui_core
