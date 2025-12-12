#pragma once

#include "BindlessScene.hpp"
#include "Config.hpp"
#include "DebugWidget.hpp"
#include "InfoDialog.hpp"
#include "OcclusionCulling.hpp"
#include "RayTracingScene.hpp"
#include "RenderSurface.hpp"
#include "Renderer.hpp"
#include "RenderingJob.hpp"
#include "Scene.hpp"
#include "UpdateUserInterfaceJob.hpp"
#include "UpdateViewParamsJob.hpp"

#include "triglav/Logging.hpp"
#include "triglav/desktop/Desktop.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/render_core/GlyphAtlas.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/render_core/PipelineCache.hpp"
#include "triglav/render_core/ResourceStorage.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <optional>

namespace triglav::render_core {
class ResourceStorage;
}

namespace triglav::renderer {

class Renderer final : public IRenderer
{
   TG_DEFINE_LOG_CATEGORY(Renderer)
 public:
   enum class Moving
   {
      None,
      Forward,
      Backwards,
      Left,
      Right,
      Up,
      Down
   };
   using Self = Renderer;

   Renderer(desktop::ISurface& desktop_surface, graphics_api::Surface& surface, graphics_api::Device& device,
            resource::ResourceManager& resource_manager, const graphics_api::Resolution& resolution);

   void update_debug_info(bool is_first_frame);
   void on_render();
   void on_resize(uint32_t width, uint32_t height);
   void on_close();
   void on_mouse_move(Vector2 position);
   void on_mouse_relative_move(float dx, float dy);
   void on_mouse_is_pressed(desktop::MouseButton button, Vector2 position);
   void on_mouse_is_released(desktop::MouseButton button, Vector2 position);
   void on_key_pressed(desktop::Key key);
   void on_key_released(desktop::Key key);
   [[nodiscard]] resource::ResourceManager& resource_manager() const;
   [[nodiscard]] std::tuple<uint32_t, uint32_t> screen_resolution() const;
   [[nodiscard]] graphics_api::Device& device() const;
   void on_config_property_changed(ConfigProperty property, const Config& config);
   void recreate_render_jobs() override;

 private:
   void update_uniform_data(float delta_time);
   static float calculate_frame_duration();
   glm::vec3 moving_direction();
   void recreate_jobs();

 private:
   bool m_must_recreate_jobs{false};
   bool m_show_debug_lines{false};
   glm::vec3 m_motion{};
   glm::vec2 m_mouse_offset{};
   bool m_on_ground{false};
   Moving m_move_direction{Moving::None};

   graphics_api::Device& m_device;
   resource::ResourceManager& m_resource_manager;

   ConfigManager m_config_manager;
   Scene m_scene;
   BindlessScene m_bindless_scene;
   render_core::GlyphCache m_glyph_cache;
   ui_core::Viewport m_ui_viewport;
   ui_core::Context m_ui_context;
   InfoDialog m_info_dialog;
   std::optional<RayTracingScene> m_ray_tracing_scene;

   render_core::ResourceStorage m_resource_storage;
   RenderSurface m_render_surface;
   render_core::PipelineCache m_pipeline_cache;
   render_core::JobGraph m_job_graph;
   UpdateViewParamsJob m_update_view_params_job;
   UpdateUserInterfaceJob m_update_user_interface_job;
   OcclusionCulling m_occlusion_culling;
   RenderingJob m_rendering_job;
   u32 m_frame_index{0};

   DebugWidget m_debug_widget;

   TG_SINK(ConfigManager, OnPropertyChanged);
};

}// namespace triglav::renderer