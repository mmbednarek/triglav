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
      std::vector<String> tabNames;
      u32 activeTab = 0;
   };

   struct Measure
   {
      Vector2 size;
      std::vector<float> tab_widths;
      float tab_height;
   };

   TabView(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   bool on_mouse_moved(const ui_core::Event&);
   bool on_mouse_pressed(const ui_core::Event&, const ui_core::Event::Mouse&);
   bool on_mouse_released(const ui_core::Event&, const ui_core::Event::Mouse&);

   void set_active_tab(u32 activeTab);

 private:
   [[nodiscard]] std::pair<float, u32> index_from_mouse_position(Vector2 position) const;
   [[nodiscard]] const Measure& get_measure(Vector2 availableSize) const;

   State m_state;
   std::vector<ui_core::TextId> m_labels;
   std::map<float, u32> m_offsetToItem;
   Vector4 m_dimensions;
   Vector4 m_croppingMask;

   ui_core::RectId m_backgroundId{};
   ui_core::RectId m_hoverRectId{};
   ui_core::RectId m_activeRectId{};
   ui_core::RectId m_highlightRectId{};
   u32 m_hoveredItem = 0;
   bool m_isDragging{false};

   mutable Vector2 m_cachedMeasureSize{};
   mutable std::optional<Measure> m_cachedMeasure{};
};

}// namespace triglav::desktop_ui
