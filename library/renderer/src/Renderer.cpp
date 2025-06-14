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
#include <spdlog/spdlog.h>

using triglav::ResourceType;
using triglav::desktop::Key;
using triglav::graphics_api::AttachmentAttribute;
using triglav::graphics_api::DeviceFeature;
using triglav::graphics_api::SampleCount;
using triglav::render_core::checkResult;
using triglav::render_core::checkStatus;
using triglav::resource::ResourceManager;
using namespace triglav::name_literals;

namespace triglav::renderer {

namespace {

graphics_api::PresentMode get_present_mode()
{
   const auto presentModeStr = io::CommandLine::the().arg("presentMode"_name);
   if (not presentModeStr.has_value())
      return graphics_api::PresentMode::Fifo;

   if (*presentModeStr == "mailbox")
      return graphics_api::PresentMode::Mailbox;

   if (*presentModeStr == "immediate")
      return graphics_api::PresentMode::Immediate;

   return graphics_api::PresentMode::Fifo;
}

}// namespace

Renderer::Renderer(desktop::ISurface& desktopSurface, graphics_api::Surface& surface, graphics_api::Device& device,
                   ResourceManager& resourceManager, const graphics_api::Resolution& resolution) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_configManager(m_device),
    m_scene(m_resourceManager),
    m_bindlessScene(m_device, m_resourceManager, m_scene),
    m_glyphCache(m_device, m_resourceManager),
    m_uiViewport({resolution.width, resolution.height}),
    m_uiContext(m_uiViewport, m_glyphCache, m_resourceManager),
    m_infoDialog(m_uiContext, m_configManager, desktopSurface),
    m_resourceStorage(m_device),
    m_renderSurface(m_device, desktopSurface, surface, m_resourceStorage, {resolution.width, resolution.height}, get_present_mode()),
    m_pipelineCache(m_device, m_resourceManager),
    m_jobGraph(m_device, m_resourceManager, m_pipelineCache, m_resourceStorage, {resolution.width, resolution.height}),
    m_updateViewParamsJob(m_scene),
    m_updateUserInterfaceJob(m_device, m_glyphCache, m_uiViewport, m_resourceManager),
    m_occlusionCulling(m_updateViewParamsJob, m_bindlessScene),
    m_renderingJob(m_configManager.config()),
    m_debugWidget(m_uiContext),
    TG_CONNECT(m_configManager, OnPropertyChanged, on_config_property_changed)
{
   if (m_device.enabled_features() & DeviceFeature::RayTracing) {
      m_rayTracingScene.emplace(m_device, m_resourceManager, m_scene);
   }

   m_infoDialog.add_to_viewport({});
   m_scene.load_level("level/wierd_tube.level"_rc);

   {
      // quickly copy visible sceen objects
      const auto copyObjectsCmdList = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Transfer));
      GAPI_CHECK_STATUS(copyObjectsCmdList.begin());

      m_bindlessScene.on_update_scene(copyObjectsCmdList);

      GAPI_CHECK_STATUS(copyObjectsCmdList.finish());

      const auto fence = GAPI_CHECK(m_device.create_fence());
      fence.await();

      const graphics_api::SemaphoreArray empty;
      GAPI_CHECK_STATUS(m_device.submit_command_list(copyObjectsCmdList, empty, empty, &fence, graphics_api::WorkType::Transfer));

      fence.await();
   }

   if (m_rayTracingScene.has_value()) {
      m_rayTracingScene->build_acceleration_structures();
   }

   m_scene.update_shadow_maps();

   m_renderingJob.emplace_stage<stage::GBufferStage>(m_device, m_bindlessScene);
   m_renderingJob.emplace_stage<stage::AmbientOcclusionStage>(m_device);
   m_renderingJob.emplace_stage<stage::ShadowMapStage>(m_scene, m_bindlessScene, m_updateViewParamsJob);
   if (m_device.enabled_features() & DeviceFeature::RayTracing) {
      m_renderingJob.emplace_stage<stage::RayTracingStage>(*m_rayTracingScene);
   }
   m_renderingJob.emplace_stage<stage::ShadingStage>();
   m_renderingJob.emplace_stage<stage::PostProcessStage>(m_updateUserInterfaceJob);

   auto& updateViewParamsCtx = m_jobGraph.add_job(UpdateViewParamsJob::JobName);
   m_updateViewParamsJob.build_job(updateViewParamsCtx);

   auto& updateUserInterfaceCtx = m_jobGraph.add_job(UpdateUserInterfaceJob::JobName);
   m_updateUserInterfaceJob.build_job(updateUserInterfaceCtx);

   auto& renderingCtx = m_jobGraph.add_job(RenderingJob::JobName);
   m_renderingJob.build_job(renderingCtx);

   m_jobGraph.add_dependency_to_previous_frame(UpdateViewParamsJob::JobName, UpdateViewParamsJob::JobName);
   m_jobGraph.add_dependency_to_previous_frame(UpdateUserInterfaceJob::JobName, UpdateUserInterfaceJob::JobName);
   m_jobGraph.add_dependency(RenderingJob::JobName, UpdateViewParamsJob::JobName);
   m_jobGraph.add_dependency(RenderingJob::JobName, UpdateUserInterfaceJob::JobName);

   RenderSurface::add_present_jobs(m_jobGraph, RenderingJob::JobName);

   m_jobGraph.build_jobs(RenderingJob::JobName);

   m_renderSurface.recreate_present_jobs();

   stage::ShadingStage::initialize_particles(m_jobGraph);

   StatisticManager::the().initialize();

   m_scene.update_shadow_maps();

   m_infoDialog.init_config_labels();
}

void Renderer::update_debug_info(const bool isFirstFrame)
{
   m_infoDialog.set_fps(StatisticManager::the().value(Stat::FramesPerSecond));
   m_infoDialog.set_min_fps(StatisticManager::the().min(Stat::FramesPerSecond));
   m_infoDialog.set_max_fps(StatisticManager::the().max(Stat::FramesPerSecond));
   m_infoDialog.set_avg_fps(StatisticManager::the().average(Stat::FramesPerSecond));
   m_infoDialog.set_gpu_time(StatisticManager::the().value(Stat::GBufferGpuTime));

   if (!isFirstFrame) {
      m_infoDialog.set_triangle_count(m_resourceStorage.pipeline_stats().get_int(0));
   }

   m_infoDialog.set_camera_pos(m_scene.camera().position());
   m_infoDialog.set_orientation({m_scene.pitch(), m_scene.yaw()});
}

void Renderer::on_render()
{
   static bool isFirstFrame = true;

   const auto deltaTime = calculate_frame_duration();

   if (m_mustRecreateJobs) {
      this->recreate_jobs();
   }

   if (m_rayTracingScene.has_value()) {
      m_rayTracingScene->build_acceleration_structures();
   }
   this->update_debug_info(isFirstFrame);
   this->update_uniform_data(deltaTime);

   if (not isFirstFrame) {
      StatisticManager::the().push_accumulated(Stat::FramesPerSecond, 1.0f / deltaTime);
      StatisticManager::the().push_accumulated(Stat::GBufferGpuTime, m_resourceStorage.timestamps().get_difference(0, 1));
   } else {
      isFirstFrame = false;
      OcclusionCulling::reset_buffers(m_device, m_jobGraph);
   }

   m_renderSurface.await_for_frame(m_frameIndex);

   m_updateViewParamsJob.prepare_frame(m_jobGraph, m_frameIndex, deltaTime);
   m_updateUserInterfaceJob.prepare_frame(m_jobGraph, m_frameIndex);

   m_jobGraph.build_semaphores();

   m_jobGraph.execute(RenderingJob::JobName, m_frameIndex, nullptr);

   m_renderSurface.present(m_jobGraph, m_frameIndex);

   StatisticManager::the().tick();

   m_frameIndex = (m_frameIndex + 1) % render_core::FRAMES_IN_FLIGHT_COUNT;
}

void Renderer::on_close()
{
   m_device.await_all();
}

void Renderer::on_mouse_move(const Vector2 position)
{
   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MouseMoved;
   event.mousePosition = position;
   event.parentSize = m_renderSurface.resolution();
   m_infoDialog.on_event(event);
}

void Renderer::on_mouse_relative_move(const float dx, const float dy)
{
   m_mouseOffset += glm::vec2{-dx * 0.1f, dy * 0.1f};
}

void Renderer::on_mouse_is_pressed(const desktop::MouseButton button, const Vector2 position)
{
   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MousePressed;
   event.mousePosition = position;
   event.parentSize = m_renderSurface.resolution();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_infoDialog.on_event(event);
}

void Renderer::on_mouse_is_released(const desktop::MouseButton button, const Vector2 position)
{
   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MouseReleased;
   event.mousePosition = position;
   event.parentSize = m_renderSurface.resolution();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_infoDialog.on_event(event);
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
      m_moveDirection = dir;
   }
   if (key == Key::F3) {
      m_showDebugLines = not m_showDebugLines;
   }
   if (key == Key::F4) {
      m_configManager.toggle_ambient_occlusion();
   }
   if (key == Key::F5) {
      m_configManager.toggle_antialiasing();
   }
   if (key == Key::F6) {
      m_configManager.toggle_ui_hidden();
   }
   if (key == Key::F7) {
      m_configManager.toggle_bloom();
   }
   if (key == Key::F8) {
      m_configManager.toggle_smooth_camera();
   }
   if (key == Key::F9) {
      m_configManager.toggle_shadow_casting();
   }
   if (key == Key::F10) {
      m_configManager.toggle_rendering_particles();
   }
   if (key == Key::Space && m_motion.z == 0.0f) {
      m_motion.z += -16.0f;
      m_onGround = false;
   }
}

void Renderer::on_key_released(const Key key)
{
   const auto dir = map_direction(key);
   if (m_moveDirection == dir) {
      m_moveDirection = Moving::None;
   }
}

ResourceManager& Renderer::resource_manager() const
{
   return m_resourceManager;
}

std::tuple<uint32_t, uint32_t> Renderer::screen_resolution() const
{
   return {m_renderSurface.resolution().x, m_renderSurface.resolution().y};
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
   switch (m_moveDirection) {
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

   spdlog::info("Recreating rendering job...");

   m_jobGraph.set_screen_size(m_renderSurface.resolution());

   auto& renderingCtx = m_jobGraph.replace_job(RenderingJob::JobName);

   m_renderingJob.build_job(renderingCtx);

   m_jobGraph.rebuild_job(RenderingJob::JobName);

   m_renderSurface.recreate_present_jobs();

   m_mustRecreateJobs = false;
}

void Renderer::on_resize(const uint32_t width, const uint32_t height)
{
   if (m_renderSurface.resolution() == Vector2u{width, height})
      return;

   m_renderSurface.recreate_swapchain(Vector2u{width, height});
}

graphics_api::Device& Renderer::device() const
{
   return m_device;
}

void Renderer::on_config_property_changed(const ConfigProperty /*property*/, const Config& config)
{
   m_mustRecreateJobs = true;
   m_renderingJob.set_config(config);
}

constexpr auto g_movingSpeed = 10.0f;

void Renderer::update_uniform_data(const float deltaTime)
{
   bool updated = m_motion != glm::vec3{0, 0, 0} || m_mouseOffset != glm::vec2{0, 0};

   m_scene.camera().set_position(m_scene.camera().position() + m_motion * deltaTime);

   if (m_moveDirection != Moving::None) {
      glm::vec3 movingDir{this->moving_direction()};
      movingDir.z = 0.0f;
      movingDir = glm::normalize(movingDir);
      m_scene.camera().set_position(m_scene.camera().position() + movingDir * (g_movingSpeed * deltaTime));
      updated = true;
   }

   if (m_scene.camera().position().z > -5.0f) {
      m_motion = glm::vec3{0.0f};
      glm::vec3 camPos{m_scene.camera().position()};
      camPos.z = -5.0f;
      m_scene.camera().set_position(camPos);
      m_onGround = true;
      updated = true;
   } else if (!m_onGround) {
      m_motion.z += 30.0f * deltaTime;
   }

   if (m_configManager.config().isSmoothCameraEnabled) {
      m_scene.update_orientation(m_mouseOffset.x * deltaTime, m_mouseOffset.y * deltaTime);
      m_mouseOffset += m_mouseOffset * (static_cast<float>(pow(0.5f, 50.0f * deltaTime)) - 1.0f);
      if (m_mouseOffset.x < 0.0001f && m_mouseOffset.y < 0.0001f) {
         m_mouseOffset = glm::vec2{0.0f};
      }
   } else {
      m_scene.update_orientation(0.05f * m_mouseOffset.x, 0.05f * m_mouseOffset.y);
      m_mouseOffset = {0.0f, 0.0f};
   }

   graphics_api::Resolution res{m_renderSurface.resolution().x, m_renderSurface.resolution().y};
   m_scene.update(res);

   if (updated) {
      m_scene.update_shadow_maps();
      m_scene.send_view_changed();
   }
}

}// namespace triglav::renderer
