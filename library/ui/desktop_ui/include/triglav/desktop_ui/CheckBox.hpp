#pragma once

#include "triglav/event/Delegate.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/PrimitiveHelpers.hpp"
#include "triglav/ui_core/Primitives.hpp"

namespace triglav::desktop_ui {
class DesktopUIManager;

class CheckBox;

class RadioGroup
{
 public:
   using Self = RadioGroup;

   TG_EVENT(OnSelection, u32)

   void add_check_box(CheckBox* cb);
   void set_active(const CheckBox* active_cb) const;

 private:
   std::vector<CheckBox*> m_check_boxes;
};

class CheckBox : public ui_core::ContainerWidget
{
 public:
   struct State
   {
      DesktopUIManager* manager;
      RadioGroup* radio_group;
      bool is_enabled;
   };
   using Self = CheckBox;

   TG_EVENT(OnStateIsChanged, bool)

   CheckBox(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   bool on_mouse_released(const ui_core::Event&, const ui_core::Event::Mouse&);
   bool on_mouse_entered(const ui_core::Event&);
   bool on_mouse_left(const ui_core::Event&);

   void set_state(bool is_enabled);

 private:
   State m_state;
   ui_core::RectInstance m_background{};
};

}// namespace triglav::desktop_ui