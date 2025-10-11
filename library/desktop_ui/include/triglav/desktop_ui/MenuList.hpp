#pragma once

#include "MenuController.hpp"

#include "triglav/Name.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/Primitives.hpp"

#include <optional>

namespace triglav::render_core {
class GlyphCache;
}

namespace triglav::desktop_ui {

class Dialog;
class DesktopUIManager;

class MenuList final : public ui_core::BaseWidget, ui_core::EventVisitor
{
 public:
   struct State
   {
      DesktopUIManager* manager;
      MenuController* controller;
      Name listName;
      Vector2 screenOffset;
   };

   struct Measure
   {
      Vector2 size;
      Vector2 item_size;
      float text_height;
      float separation_height;
      u32 separator_count;
   };

   MenuList(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   void on_mouse_moved(const ui_core::Event&) override;
   void on_mouse_released(const ui_core::Event&, const ui_core::Event::Mouse&) override;

 private:
   [[nodiscard]] std::pair<float, Name> index_from_mouse_position(Vector2 position) const;
   [[nodiscard]] Measure get_measure() const;

   ui_core::Context& m_context;
   State m_state;
   std::vector<ui_core::TextId> m_labels;
   std::vector<ui_core::SpriteId> m_icons;
   std::vector<ui_core::RectId> m_separators;
   std::map<float, Name> m_heightToItem;
   ui_core::RectId m_backgroundId;
   ui_core::RectId m_hoverRectId{};
   Vector4 m_croppingMask;
   Name m_hoveredItem = 0;
   Dialog* m_subMenu = nullptr;

   mutable std::optional<Measure> m_cachedMeasure{};
};

}// namespace triglav::desktop_ui