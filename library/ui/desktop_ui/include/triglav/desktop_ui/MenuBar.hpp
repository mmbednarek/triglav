#pragma once

#include "MenuController.hpp"

#include "triglav/Name.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/Primitives.hpp"

#include <optional>

namespace triglav::desktop_ui {

class Dialog;
class DesktopUIManager;

class MenuBar final : public ui_core::BaseWidget
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

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   void on_mouse_moved(const ui_core::Event&);
   void on_mouse_pressed(const ui_core::Event&, const ui_core::Event::Mouse&);
   void on_mouse_left(const ui_core::Event&);

 private:
   [[nodiscard]] std::pair<float, Name> index_from_mouse_position(Vector2 position) const;
   void close_submenu();
   [[nodiscard]] Measure get_measure() const;

   ui_core::Context& m_context;
   State m_state;
   std::vector<ui_core::TextId> m_labels;
   std::map<float, Name> m_offset_to_item;
   ui_core::RectId m_background_id{};
   ui_core::RectId m_hover_rect_id{};
   Vector4 m_dimensions;
   Vector4 m_cropping_mask;
   Name m_hovered_item = 0;
   float m_hovered_item_offset = 0.0f;
   Dialog* m_sub_menu = nullptr;

   mutable std::optional<Measure> m_cached_measure{};
};

}// namespace triglav::desktop_ui
