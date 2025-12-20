#pragma once

#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/desktop_ui/WidgetRenderer.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/render_core/IRenderer.hpp"
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

class RootWindow final : public render_core::IRenderer
{
 public:
   using Self = RootWindow;

   RootWindow(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
              render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager);

   void initialize();
   void uninitialize() const;
   void update();
   void on_close();
   void on_resize(Vector2i size);

   void set_render_viewport(RenderViewport* viewport);

   template<ui_core::ConstructableWidget T>
   T& create_root_widget(typename T::State&& state)
   {
      return m_widget_renderer.create_root_widget<T>(std::forward<typename T::State>(state));
   }

   [[nodiscard]] graphics_api::Device& device() const;
   [[nodiscard]] desktop::ISurface& surface() const;
   [[nodiscard]] resource::ResourceManager& resource_manager() const;
   [[nodiscard]] bool should_close() const;
   void recreate_render_jobs() override;

 private:
   void build_rendering_job(render_core::BuildContext& ctx);

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resource_manager;

   std::shared_ptr<desktop::ISurface> m_surface;
   graphics_api::Surface m_graphics_surface;
   render_core::ResourceStorage m_resource_storage;
   renderer::RenderSurface m_render_surface;
   desktop_ui::WidgetRenderer m_widget_renderer;
   render_core::PipelineCache m_pipeline_cache;
   render_core::JobGraph m_job_graph;
   RenderViewport* m_render_viewport{};
   u32 m_frame_index{0};
   bool m_should_close{false};
   bool m_is_initialized{false};
   bool m_should_update_viewport{false};

   TG_SINK(desktop::ISurface, OnClose);
   TG_SINK(desktop::ISurface, OnResize);
};

}// namespace triglav::editor
