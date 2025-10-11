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
   };

   MenuList(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   void on_mouse_moved(const ui_core::Event&) override;
   void on_mouse_released(const ui_core::Event&, const ui_core::Event::Mouse&) override;

 private:
   [[nodiscard]] u32 index_from_mouse_position(Vector2 position) const;
   [[nodiscard]] Measure get_measure() const;

   ui_core::Context& m_context;
   State m_state;
   std::vector<ui_core::TextId> m_labels;
   std::vector<ui_core::SpriteId> m_icons;
   ui_core::RectId m_backgroundId;
   ui_core::RectId m_hoverRectId{};
   Vector4 m_croppingMask;
   u32 m_hoverIndex = ~0;
   Dialog* m_subMenu = nullptr;

   mutable std::optional<Measure> m_cachedMeasure{};
};

}// namespace triglav::desktop_ui