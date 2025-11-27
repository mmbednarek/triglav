#pragma once

#include "WidgetRenderer.hpp"

#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/renderer/RenderSurface.hpp"
#include "triglav/renderer/UpdateUserInterfaceJob.hpp"
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

   // Top level dialog
   Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
          render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager, Vector2u dimensions, StringView title);

   // Popup dialog
   Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::ISurface& parent_surface,
          render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager, Vector2u dimensions, Vector2i offset);

   void initialize();
   void uninitialize() const;
   void update();
   void on_close();
   void on_resize(Vector2i size);
   [[nodiscard]] WidgetRenderer& widget_renderer();

   template<ui_core::ConstructableWidget T>
   T& create_root_widget(typename T::State&& state)
   {
      return m_widget_renderer.create_root_widget<T>(std::forward<typename T::State>(state));
   }

   template<typename T, typename... TArgs>
   T& emplace_root_widget(TArgs&&... args)
   {
      return m_widget_renderer.emplace_root_widget<T>(std::forward<TArgs>(args)...);
   }

   [[nodiscard]] desktop::ISurface& surface() const;
   [[nodiscard]] bool should_close() const;

 private:
   Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, render_core::GlyphCache& glyph_cache,
          resource::ResourceManager& resource_manager, Vector2u dimensions, std::shared_ptr<desktop::ISurface> surface);

   void build_rendering_job(render_core::BuildContext& ctx);

   graphics_api::Device& m_device;

   std::shared_ptr<desktop::ISurface> m_surface;
   graphics_api::Surface m_graphics_surface;
   render_core::ResourceStorage m_resource_storage;
   renderer::RenderSurface m_render_surface;
   WidgetRenderer m_widget_renderer;
   render_core::PipelineCache m_pipeline_cache;
   render_core::JobGraph m_job_graph;
   u32 m_frame_index{0};
   bool m_should_close{false};
   bool m_is_initialized{false};

   TG_SINK(desktop::ISurface, OnClose);
   TG_SINK(desktop::ISurface, OnResize);
};

}// namespace triglav::desktop_ui
