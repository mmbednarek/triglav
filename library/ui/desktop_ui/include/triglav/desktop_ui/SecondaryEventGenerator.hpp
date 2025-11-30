#pragma once

#include "triglav/desktop/ISurface.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {

class SecondaryEventGenerator final : public ui_core::ProxyWidget
{
 public:
   SecondaryEventGenerator(ui_core::Context& ctx, ui_core::IWidget* parent, desktop::ISurface& surface);

   void on_event(const ui_core::Event& event) override;

   void on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& mouse) const;
   void on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard);

 private:
   void forward_event(const ui_core::Event& event);

   desktop::ISurface& m_surface;
};

}// namespace triglav::desktop_ui