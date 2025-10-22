#pragma once

#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/desktop_ui/WidgetRenderer.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/renderer/RenderSurface.hpp"
#include "triglav/renderer/UpdateUserInterfaceJob.hpp"
#include "triglav/ui_core/IWidget.hpp"

#include <memory>

namespace triglav::render_core {
class BuildContext;
}
namespace triglav::editor {

class RenderViewport;

class RootWindow
{
 public:
   using Self = RootWindow;

   RootWindow(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
              render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager);

   void initialize();
   void uninitialize() const;
   void update();
   void on_close();
   void on_resize(Vector2i size);

   void set_render_viewport(RenderViewport* viewport);

   template<ui_core::ConstructableWidget T>
   T& create_root_widget(typename T::State&& state)
   {
      return m_widgetRenderer.create_root_widget<T>(std::forward<typename T::State>(state));
   }

   [[nodiscard]] graphics_api::Device& device() const;
   [[nodiscard]] desktop::ISurface& surface() const;
   [[nodiscard]] bool should_close() const;

 private:
   void build_rendering_job(render_core::BuildContext& ctx);

   graphics_api::Device& m_device;

   std::shared_ptr<desktop::ISurface> m_surface;
   graphics_api::Surface m_graphicsSurface;
   render_core::ResourceStorage m_resourceStorage;
   renderer::RenderSurface m_renderSurface;
   desktop_ui::WidgetRenderer m_widgetRenderer;
   render_core::PipelineCache m_pipelineCache;
   render_core::JobGraph m_jobGraph;
   RenderViewport* m_renderViewport{};
   u32 m_frameIndex{0};
   bool m_shouldClose{false};
   bool m_isInitialized{false};
   bool m_shouldUpdateViewport{false};

   TG_SINK(desktop::ISurface, OnClose);
   TG_SINK(desktop::ISurface, OnResize);
};

}// namespace triglav::editor
