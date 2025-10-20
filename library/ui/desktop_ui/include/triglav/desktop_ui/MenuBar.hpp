#pragma once

#include "MenuController.hpp"

#include "triglav/Name.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/Primitives.hpp"

#include <optional>

namespace triglav::desktop_ui {

class Dialog;
class DesktopUIManager;

class MenuBar final : public ui_core::BaseWidget, ui_core::EventVisitor
{
 public:
   struct State
   {
      DesktopUIManager* manager;
      MenuController* controller;
   };

   struct Measure
   {
      Vector2 size;
      std::map<Name, float> item_widths;
      float item_height;
   };

   MenuBar(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   bool on_mouse_moved(const ui_core::Event&) override;
   bool on_mouse_pressed(const ui_core::Event&, const ui_core::Event::Mouse&) override;
   bool on_mouse_left(const ui_core::Event&) override;

 private:
   [[nodiscard]] std::pair<float, Name> index_from_mouse_position(Vector2 position) const;
   void close_submenu();
   [[nodiscard]] Measure get_measure() const;

   ui_core::Context& m_context;
   State m_state;
   std::vector<ui_core::TextId> m_labels;
   std::map<float, Name> m_offsetToItem;
   ui_core::RectId m_backgroundId{};
   ui_core::RectId m_hoverRectId{};
   Vector4 m_dimensions;
   Vector4 m_croppingMask;
   Name m_hoveredItem = 0;
   float m_hoveredItemOffset = 0.0f;
   Dialog* m_subMenu = nullptr;

   mutable std::optional<Measure> m_cachedMeasure{};
};

}// namespace triglav::desktop_ui
