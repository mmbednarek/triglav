#include "Renderer.hpp"

#include "Config.hpp"
#include "StatisticManager.hpp"
#include "stage/AmbientOcclusionStage.hpp"
#include "stage/GBufferStage.hpp"
#include "stage/PostProcessStage.hpp"
#include "stage/RayTracingStage.hpp"
#include "stage/ShadingStage.hpp"
#include "stage/ShadowMapStage.hpp"

#include "triglav/Name.hpp"
#include "triglav/Ranges.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/io/CommandLine.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/render_core/ResourceStorage.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <chrono>
#include <cmath>
#include <glm/glm.hpp>

using triglav::ResourceType;
using triglav::desktop::Key;
using triglav::graphics_api::AttachmentAttribute;
using triglav::graphics_api::DeviceFeature;
using triglav::graphics_api::SampleCount;
using triglav::render_core::check_result;
using triglav::render_core::check_status;
using triglav::resource::ResourceManager;
using namespace triglav::name_literals;

namespace triglav::renderer {

namespace {

graphics_api::PresentMode get_present_mode()
{
   const auto present_mode_str = io::CommandLine::the().arg("presentMode"_name);
   if (not present_mode_str.has_value())
      return graphics_api::PresentMode::Fifo;

   if (*present_mode_str == "mailbox")
      return graphics_api::PresentMode::Mailbox;

   if (*present_mode_str == "immediate")
      return graphics_api::PresentMode::Immediate;

   return graphics_api::PresentMode::Fifo;
}

}// namespace

Renderer::Renderer(desktop::ISurface& desktop_surface, graphics_api::Surface& surface, graphics_api::Device& device,
                   ResourceManager& resource_manager, const graphics_api::Resolution& resolution) :
    m_device(device),
    m_resource_manager(resource_manager),
    m_config_manager(m_device),
    m_scene(m_resource_manager),
    m_bindless_scene(m_device, m_resource_manager, m_scene),
    m_glyph_cache(m_device, m_resource_manager),
    m_ui_viewport({resolution.width, resolution.height}),
    m_ui_context(m_ui_viewport, m_glyph_cache, m_resource_manager),
    m_info_dialog(m_ui_context, m_config_manager, desktop_surface),
    m_resource_storage(m_device),
    m_render_surface(m_device, desktop_surface, surface, m_resource_storage, {resolution.width, resolution.height}, get_present_mode()),
    m_pipeline_cache(m_device, m_resource_manager),
    m_job_graph(m_device, m_resource_manager, m_pipeline_cache, m_resource_storage, {resolution.width, resolution.height}),
    m_update_view_params_job(m_scene),
    m_update_user_interface_job(m_device, m_glyph_cache, m_ui_viewport, m_resource_manager),
    m_occlusion_culling(m_update_view_params_job, m_bindless_scene),
    m_rendering_job(m_config_manager.config()),
    m_debug_widget(m_ui_context),
    TG_CONNECT(m_config_manager, OnPropertyChanged, on_config_property_changed)
{
   if (m_device.enabled_features() & DeviceFeature::RayTracing) {
      m_ray_tracing_scene.emplace(m_device, m_resource_manager, m_scene);
   }

   m_info_dialog.add_to_viewport({}, {0, 0, resolution.width, resolution.height});
   m_scene.load_level("demo.level"_rc);

   m_bindless_scene.write_objects_to_buffer();

   if (m_ray_tracing_scene.has_value()) {
      m_ray_tracing_scene->build_acceleration_structures();
   }

   m_scene.update_shadow_maps();

   m_rendering_job.emplace_stage<stage::GBufferStage>(m_device, m_bindless_scene);
   m_rendering_job.emplace_stage<stage::AmbientOcclusionStage>(m_device);
   m_rendering_job.emplace_stage<stage::ShadowMapStage>(m_scene, m_bindless_scene, m_update_view_params_job);
   if (m_device.enabled_features() & DeviceFeature::RayTracing) {
      m_rendering_job.emplace_stage<stage::RayTracingStage>(*m_ray_tracing_scene);
   }
   m_rendering_job.emplace_stage<stage::ShadingStage>();
   m_rendering_job.emplace_stage<stage::PostProcessStage>(&m_update_user_interface_job);

   auto& update_view_params_ctx = m_job_graph.add_job(UpdateViewParamsJob::JobName);
   m_update_view_params_job.build_job(update_view_params_ctx);

   auto& update_user_interface_ctx = m_job_graph.add_job(UpdateUserInterfaceJob::JobName);
   m_update_user_interface_job.build_job(update_user_interface_ctx);

   auto& rendering_ctx = m_job_graph.add_job(RenderingJob::JobName);
   m_rendering_job.build_job(rendering_ctx);

   m_job_graph.add_dependency_to_previous_frame(UpdateViewParamsJob::JobName, UpdateViewParamsJob::JobName);
   m_job_graph.add_dependency_to_previous_frame(UpdateUserInterfaceJob::JobName, UpdateUserInterfaceJob::JobName);
   m_job_graph.add_dependency(RenderingJob::JobName, UpdateViewParamsJob::JobName);
   m_job_graph.add_dependency(RenderingJob::JobName, UpdateUserInterfaceJob::JobName);

   RenderSurface::add_present_jobs(m_job_graph, RenderingJob::JobName);

   m_job_graph.build_jobs(RenderingJob::JobName);

   m_render_surface.recreate_present_jobs();

   stage::ShadingStage::initialize_particles(m_job_graph);

   StatisticManager::the().initialize();

   m_scene.update_shadow_maps();

   m_info_dialog.init_config_labels();
}

void Renderer::update_debug_info(const bool is_first_frame)
{
   m_info_dialog.set_fps(StatisticManager::the().value(Stat::FramesPerSecond));
   m_info_dialog.set_min_fps(StatisticManager::the().min(Stat::FramesPerSecond));
   m_info_dialog.set_max_fps(StatisticManager::the().max(Stat::FramesPerSecond));
   m_info_dialog.set_avg_fps(StatisticManager::the().average(Stat::FramesPerSecond));
   m_info_dialog.set_gpu_time(StatisticManager::the().value(Stat::GBufferGpuTime));

   if (!is_first_frame) {
      m_info_dialog.set_triangle_count(m_resource_storage.pipeline_stats().get_int(0));
   }

   m_info_dialog.set_camera_pos(m_scene.camera().position());
   m_info_dialog.set_orientation({m_scene.pitch(), m_scene.yaw()});
}

void Renderer::on_render()
{
   static bool is_first_frame = true;

   const auto delta_time = calculate_frame_duration();

   if (m_must_recreate_jobs) {
      this->recreate_jobs();
   }

   if (m_ray_tracing_scene.has_value()) {
      m_ray_tracing_scene->build_acceleration_structures();
   }
   this->update_debug_info(is_first_frame);
   this->update_uniform_data(delta_time);

   if (not is_first_frame) {
      StatisticManager::the().push_accumulated(Stat::FramesPerSecond, 1.0f / delta_time);
      StatisticManager::the().push_accumulated(Stat::GBufferGpuTime, m_resource_storage.timestamps().get_difference(0, 1));
   } else {
      is_first_frame = false;
      OcclusionCulling::reset_buffers(m_device, m_job_graph);
   }

   m_render_surface.await_for_frame(m_frame_index);

   m_update_view_params_job.prepare_frame(m_job_graph, m_frame_index, delta_time);
   m_update_user_interface_job.prepare_frame(m_job_graph, m_frame_index);

   m_job_graph.build_semaphores();

   m_job_graph.execute(RenderingJob::JobName, m_frame_index, nullptr);

   m_render_surface.present(m_job_graph, m_frame_index);

   StatisticManager::the().tick();

   m_frame_index = (m_frame_index + 1) % render_core::FRAMES_IN_FLIGHT_COUNT;
}

void Renderer::on_close()
{
   m_device.await_all();
}

void Renderer::on_mouse_move(const Vector2 position)
{
   ui_core::Event event;
   event.event_type = ui_core::Event::Type::MouseMoved;
   event.mouse_position = position;
   event.global_mouse_position = position;
   event.parent_size = m_render_surface.resolution();
   m_info_dialog.on_event(event);
}

void Renderer::on_mouse_relative_move(const float dx, const float dy)
{
   m_mouse_offset += glm::vec2{-dx * 0.1f, dy * 0.1f};
}

void Renderer::on_mouse_is_pressed(const desktop::MouseButton button, const Vector2 position)
{
   ui_core::Event event;
   event.event_type = ui_core::Event::Type::MousePressed;
   event.mouse_position = position;
   event.global_mouse_position = position;
   event.parent_size = m_render_surface.resolution();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_info_dialog.on_event(event);
}

void Renderer::on_mouse_is_released(const desktop::MouseButton button, const Vector2 position)
{
   ui_core::Event event;
   event.event_type = ui_core::Event::Type::MouseReleased;
   event.mouse_position = position;
   event.global_mouse_position = position;
   event.parent_size = m_render_surface.resolution();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_info_dialog.on_event(event);
}

static Renderer::Moving map_direction(const Key key)
{
   switch (key) {
   case Key::W:
      return Renderer::Moving::Forward;
   case Key::S:
      return Renderer::Moving::Backwards;
   case Key::A:
      return Renderer::Moving::Left;
   case Key::D:
      return Renderer::Moving::Right;
   default:
      break;
      //   case Key::Q: return Renderer::Moving::Up;
      //   case Key::E: return Renderer::Moving::Down;
   }

   return Renderer::Moving::None;
}

void Renderer::on_key_pressed(const Key key)
{
   if (const auto dir = map_direction(key); dir != Moving::None) {
      m_move_direction = dir;
   }
   if (key == Key::F3) {
      m_show_debug_lines = not m_show_debug_lines;
   }
   if (key == Key::F4) {
      m_config_manager.toggle_ambient_occlusion();
   }
   if (key == Key::F5) {
      m_config_manager.toggle_antialiasing();
   }
   if (key == Key::F6) {
      m_config_manager.toggle_ui_hidden();
   }
   if (key == Key::F7) {
      m_config_manager.toggle_bloom();
   }
   if (key == Key::F8) {
      m_config_manager.toggle_smooth_camera();
   }
   if (key == Key::F9) {
      m_config_manager.toggle_shadow_casting();
   }
   if (key == Key::F10) {
      m_config_manager.toggle_rendering_particles();
   }
   if (key == Key::Space && m_motion.z == 0.0f) {
      m_motion.z += -16.0f;
      m_on_ground = false;
   }
}

void Renderer::on_key_released(const Key key)
{
   const auto dir = map_direction(key);
   if (m_move_direction == dir) {
      m_move_direction = Moving::None;
   }
}

ResourceManager& Renderer::resource_manager() const
{
   return m_resource_manager;
}

std::tuple<uint32_t, uint32_t> Renderer::screen_resolution() const
{
   return {m_render_surface.resolution().x, m_render_surface.resolution().y};
}

float Renderer::calculate_frame_duration()
{
   static std::chrono::steady_clock::time_point last{std::chrono::steady_clock::now()};

   const auto now = std::chrono::steady_clock::now();
   const auto diff = now - last;
   last = now;

   return static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(diff).count()) / 1000000.0f;
}

glm::vec3 Renderer::moving_direction()
{
   switch (m_move_direction) {
   case Moving::None:
      break;
   case Moving::Forward:
      return m_scene.camera().orientation() * glm::vec3{0.0f, 1.0f, 0.0f};
   case Moving::Backwards:
      return m_scene.camera().orientation() * glm::vec3{0.0f, -1.0f, 0.0f};
   case Moving::Left:
      return m_scene.camera().orientation() * glm::vec3{-1.0f, 0.0f, 0.0f};
   case Moving::Right:
      return m_scene.camera().orientation() * glm::vec3{1.0f, 0.0f, 0.0f};
   case Moving::Up:
      return glm::vec3{0.0f, 0.0f, -1.0f};
   case Moving::Down:
      return glm::vec3{0.0f, 0.0f, 1.0f};
   }

   return glm::vec3{0.0f, 0.0f, 0.0f};
}

void Renderer::recreate_jobs()
{
   m_device.await_all();

   log_info("Recreating rendering job...");

   m_job_graph.set_screen_size(m_render_surface.resolution());

   auto& rendering_ctx = m_job_graph.replace_job(RenderingJob::JobName);

   m_rendering_job.build_job(rendering_ctx);

   m_job_graph.rebuild_job(RenderingJob::JobName);

   m_render_surface.recreate_present_jobs();

   m_must_recreate_jobs = false;
}

void Renderer::on_resize(const uint32_t width, const uint32_t height)
{
   if (m_render_surface.resolution() == Vector2u{width, height})
      return;

   m_render_surface.recreate_swapchain(Vector2u{width, height});
   this->recreate_jobs();
}

graphics_api::Device& Renderer::device() const
{
   return m_device;
}

void Renderer::on_config_property_changed(const ConfigProperty /*property*/, const Config& config)
{
   m_must_recreate_jobs = true;
   m_rendering_job.set_config(config);
}

constexpr auto g_moving_speed = 10.0f;

void Renderer::update_uniform_data(const float delta_time)
{
   bool updated = m_motion != glm::vec3{0, 0, 0} || m_mouse_offset != glm::vec2{0, 0};

   m_scene.camera().set_position(m_scene.camera().position() + m_motion * delta_time);

   if (m_move_direction != Moving::None) {
      glm::vec3 moving_dir{this->moving_direction()};
      moving_dir.z = 0.0f;
      moving_dir = glm::normalize(moving_dir);
      m_scene.camera().set_position(m_scene.camera().position() + moving_dir * (g_moving_speed * delta_time));
      updated = true;
   }

   if (m_scene.camera().position().z > -5.0f) {
      m_motion = glm::vec3{0.0f};
      glm::vec3 cam_pos{m_scene.camera().position()};
      cam_pos.z = -5.0f;
      m_scene.camera().set_position(cam_pos);
      m_on_ground = true;
      updated = true;
   } else if (!m_on_ground) {
      m_motion.z += 30.0f * delta_time;
   }

   if (m_config_manager.config().is_smooth_camera_enabled) {
      m_scene.update_orientation(m_mouse_offset.x * delta_time, m_mouse_offset.y * delta_time);
      m_mouse_offset += m_mouse_offset * (static_cast<float>(pow(0.5f, 50.0f * delta_time)) - 1.0f);
      if (m_mouse_offset.x < 0.0001f && m_mouse_offset.y < 0.0001f) {
         m_mouse_offset = glm::vec2{0.0f};
      }
   } else {
      m_scene.update_orientation(0.05f * m_mouse_offset.x, 0.05f * m_mouse_offset.y);
      m_mouse_offset = {0.0f, 0.0f};
   }

   graphics_api::Resolution res{m_render_surface.resolution().x, m_render_surface.resolution().y};
   m_scene.update(res);

   if (updated) {
      m_scene.update_shadow_maps();
      m_scene.send_view_changed();
   }
}

}// namespace triglav::renderer
