#pragma once

#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/renderer/RenderSurface.hpp"
#include "triglav/renderer/UpdateUserInterfaceJob.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <memory>

namespace triglav::render_core {
class BuildContext;
}
namespace triglav::desktop_ui {

class Dialog
{
 public:
   using Self = Dialog;

   Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
          render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager, Vector2u dimensions);

   void initialize();
   void update();
   void on_close();
   void on_mouse_move(Vector2 position);
   void on_mouse_button_is_pressed(desktop::MouseButton button);
   void on_mouse_button_is_released(desktop::MouseButton button);

   ui_core::IWidget& set_root_widget(ui_core::IWidgetPtr&& content);

   template<ui_core::ConstructableWidget T>
   T& create_root_widget(typename T::State&& state)
   {
      return dynamic_cast<T&>(this->set_root_widget(std::make_unique<T>(m_context, std::forward<typename T::State>(state), nullptr)));
   }

   [[nodiscard]] bool should_close() const;

 private:
   void build_rendering_job(render_core::BuildContext& ctx);

   render_core::GlyphCache& m_glyphCache;
   resource::ResourceManager& m_resourceManager;

   std::shared_ptr<desktop::ISurface> m_surface;
   graphics_api::Surface m_graphicsSurface;
   render_core::ResourceStorage m_resourceStorage;
   renderer::RenderSurface m_renderSurface;
   ui_core::Viewport m_uiViewport;
   renderer::UpdateUserInterfaceJob m_updateUiJob;
   render_core::PipelineCache m_pipelineCache;
   render_core::JobGraph m_jobGraph;
   ui_core::Context m_context;
   ui_core::IWidgetPtr m_rootWidget{};
   u32 m_frameIndex{0};
   bool m_shouldClose{false};
   bool m_isInitialized{false};
   Vector2 m_mousePosition{};

   TG_SINK(desktop::ISurface, OnClose);
   TG_SINK(desktop::ISurface, OnMouseMove);
   TG_SINK(desktop::ISurface, OnMouseButtonIsPressed);
   TG_SINK(desktop::ISurface, OnMouseButtonIsReleased);
};

}// namespace triglav::desktop_ui