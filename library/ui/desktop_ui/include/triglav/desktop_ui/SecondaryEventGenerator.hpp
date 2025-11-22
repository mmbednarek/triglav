#pragma once

#include "triglav/EnumFlags.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {

enum class Modifier
{
   None = 0,
   Control = 0b001,
   Shift = 0b010,
   Alt = 0b100,
   Count,
};

TRIGLAV_DECL_FLAGS(Modifier)

class SecondaryEventGenerator final : public ui_core::ProxyWidget
{
 public:
   SecondaryEventGenerator(ui_core::Context& ctx, ui_core::IWidget* parent);

   void on_event(const ui_core::Event& event) override;

   void on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& mouse) const;
   void on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard);
   void on_key_released(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard);

 private:
   void forward_event(const ui_core::Event& event);

   ModifierFlags m_modifier_state;
};

}// namespace triglav::desktop_ui