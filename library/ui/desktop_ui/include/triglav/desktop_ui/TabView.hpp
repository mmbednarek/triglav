#pragma once

#include "MenuController.hpp"

#include "triglav/Name.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/Primitives.hpp"

#include <optional>

namespace triglav::desktop_ui {

class Dialog;
class DesktopUIManager;

class TabView final : public ui_core::LayoutWidget
{
 public:
   struct State
   {
      DesktopUIManager* manager;
      std::vector<String> tab_names;
      u32 active_tab = 0;
   };

   struct Measure
   {
      Vector2 size;
      std::vector<float> tab_widths;
      float tab_height;
   };

   TabView(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   bool on_mouse_moved(const ui_core::Event&);
   bool on_mouse_pressed(const ui_core::Event&, const ui_core::Event::Mouse&);
   bool on_mouse_released(const ui_core::Event&, const ui_core::Event::Mouse&);

   void set_active_tab(u32 active_tab);

 private:
   [[nodiscard]] std::pair<float, u32> index_from_mouse_position(Vector2 position) const;
   [[nodiscard]] const Measure& get_measure(Vector2 available_size) const;

   State m_state;
   std::vector<ui_core::TextId> m_labels;
   std::map<float, u32> m_offset_to_item;
   Vector4 m_dimensions;
   Vector4 m_cropping_mask;

   ui_core::RectId m_background_id{};
   ui_core::RectId m_hover_rect_id{};
   ui_core::RectId m_active_rect_id{};
   ui_core::RectId m_highlight_rect_id{};
   u32 m_hovered_item = 0;
   bool m_is_dragging{false};

   mutable Vector2 m_cached_measure_size{};
   mutable std::optional<Measure> m_cached_measure{};
};

}// namespace triglav::desktop_ui
