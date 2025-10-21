#pragma once

#include "triglav/Utf8.hpp"
#include "triglav/desktop/Desktop.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/render_core/ResourceStorage.hpp"
#include "triglav/renderer/RenderSurface.hpp"
#include "triglav/renderer/UpdateUserInterfaceJob.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::render_core {
class GlyphCache;
}

namespace triglav::desktop_ui {

class WidgetRenderer
{
 public:
   using Self = WidgetRenderer;

   WidgetRenderer(desktop::ISurface& surface, render_core::GlyphCache& glyphCache,
                  resource::ResourceManager& resourceManager, graphics_api::Device& device);

   void create_update_job(render_core::BuildContext& ctx);
   void create_render_job(render_core::BuildContext& ctx, Name output_render_target);
   void add_widget_to_viewport(Vector2i resolution);

   void on_mouse_enter(Vector2 position);
   void on_mouse_move(Vector2 position);
   void on_mouse_wheel_turn(float amount) const;
   void on_mouse_button_is_pressed(desktop::MouseButton button) const;
   void on_mouse_button_is_released(desktop::MouseButton button) const;
   void on_key_is_pressed(desktop::Key key) const;
   void on_text_input(Rune rune) const;

   ui_core::IWidget& set_root_widget(ui_core::IWidgetPtr&& content);

   template<ui_core::ConstructableWidget T>
   T& create_root_widget(typename T::State&& state)
   {
      return dynamic_cast<T&>(this->set_root_widget(std::make_unique<T>(m_context, std::forward<typename T::State>(state), nullptr)));
   }


 private:
   desktop::ISurface& m_surface;

   ui_core::Viewport m_uiViewport;
   renderer::UpdateUserInterfaceJob m_updateUiJob;
   render_core::PipelineCache m_pipelineCache;
   ui_core::Context m_context;
   ui_core::IWidgetPtr m_rootWidget{};
   u32 m_frameIndex{0};
   bool m_shouldClose{false};
   bool m_isInitialized{false};
   Vector2 m_mousePosition{};

   TG_SINK(desktop::ISurface, OnMouseEnter);
   TG_SINK(desktop::ISurface, OnMouseMove);
   TG_SINK(desktop::ISurface, OnMouseWheelTurn);
   TG_SINK(desktop::ISurface, OnMouseButtonIsPressed);
   TG_SINK(desktop::ISurface, OnMouseButtonIsReleased);
   TG_SINK(desktop::ISurface, OnKeyIsPressed);
   TG_SINK(desktop::ISurface, OnTextInput);
};

}// namespace triglav::desktop_ui