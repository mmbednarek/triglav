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

constexpr auto g_colorFormat = GAPI_FORMAT(BGRA, sRGB);

namespace {

graphics_api::Resolution create_viewport_resolution(const graphics_api::Device& device, const graphics_api::Surface& surface,
                                                    const uint32_t width, const uint32_t height)
{
   graphics_api::Resolution resolution{
      .width = width,
      .height = height,
   };

   const auto [minResolution, maxResolution] = device.get_surface_resolution_limits(surface);
   resolution.width = std::clamp(resolution.width, minResolution.width, maxResolution.width);
   resolution.height = std::clamp(resolution.height, minResolution.height, maxResolution.height);

   return resolution;
}

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
    m_desktopSurface(desktopSurface),
    m_surface(surface),
    m_device(device),
    m_resourceManager(resourceManager),
    m_configManager(m_device),
    m_scene(m_resourceManager),
    m_bindlessScene(m_device, m_resourceManager, m_scene),
    m_resolution(create_viewport_resolution(m_device, m_surface, resolution.width, resolution.height)),
    m_swapchain(
       checkResult(m_device.create_swapchain(m_surface, g_colorFormat, graphics_api::ColorSpace::sRGB, m_resolution, get_present_mode()))),
    m_glyphCache(m_device, m_resourceManager),
    m_uiViewport({resolution.width, resolution.height}),
    m_infoDialog(m_uiViewport, m_glyphCache),
    m_rayTracingScene((m_device.enabled_features() & DeviceFeature::RayTracing)
                         ? std::make_optional<RayTracingScene>(m_device, m_resourceManager, m_scene)
                         : std::nullopt),
    m_resourceStorage(m_device),
    m_pipelineCache(m_device, m_resourceManager),
    m_jobGraph(m_device, m_resourceManager, m_pipelineCache, m_resourceStorage, {resolution.width, resolution.height}),
    m_updateViewParamsJob(m_scene),
    m_updateUserInterfaceJob(m_device, m_glyphCache, m_uiViewport),
    m_occlusionCulling(m_updateViewParamsJob, m_bindlessScene),
    m_renderingJob(m_configManager.config()),
    m_frameFences{
       GAPI_CHECK(m_device.create_fence()),
       GAPI_CHECK(m_device.create_fence()),
       GAPI_CHECK(m_device.create_fence()),
    },
    TG_CONNECT(m_configManager, OnPropertyChanged, on_config_property_changed)
{
   m_infoDialog.initialize();
   m_scene.load_level("demo.level"_rc);

   {
      // quickly copy visible sceen objects
      auto copyObjectsCmdList = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Transfer));
      GAPI_CHECK_STATUS(copyObjectsCmdList.begin());

      m_bindlessScene.on_update_scene(copyObjectsCmdList);

      GAPI_CHECK_STATUS(copyObjectsCmdList.finish());

      auto fence = GAPI_CHECK(m_device.create_fence());
      fence.await();

      graphics_api::SemaphoreArray empty;
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

   m_jobGraph.add_external_job("job.acquire_swapchain_image"_name);
   m_jobGraph.add_external_job("job.copy_present_image"_name);
   m_jobGraph.add_external_job("job.present_swapchain_image"_name);

   m_jobGraph.add_dependency_to_previous_frame(UpdateViewParamsJob::JobName, UpdateViewParamsJob::JobName);
   m_jobGraph.add_dependency_to_previous_frame(UpdateUserInterfaceJob::JobName, UpdateUserInterfaceJob::JobName);
   m_jobGraph.add_dependency(RenderingJob::JobName, UpdateViewParamsJob::JobName);
   m_jobGraph.add_dependency(RenderingJob::JobName, UpdateUserInterfaceJob::JobName);

   m_jobGraph.add_dependency("job.copy_present_image"_name, "job.acquire_swapchain_image"_name);
   m_jobGraph.add_dependency("job.copy_present_image"_name, RenderingJob::JobName);
   m_jobGraph.add_dependency("job.present_swapchain_image"_name, "job.copy_present_image"_name);

   m_jobGraph.build_jobs(RenderingJob::JobName);

   Renderer::prepare_pre_present_commands(m_device, m_swapchain, m_resourceStorage, m_prePresentCommands);

   stage::ShadingStage::initialize_particles(m_jobGraph);

   StatisticManager::the().initialize();

   m_scene.update_shadow_maps();

   this->init_config_labels();
}

void Renderer::update_debug_info(const bool isFirstFrame)
{
   const auto framerateStr = std::format("{:.1f}", StatisticManager::the().value(Stat::FramesPerSecond));
   m_uiViewport.set_text_content("info_dialog/metrics/fps/value"_name, framerateStr);

   const auto framerateMinStr = std::format("{:.1f}", StatisticManager::the().min(Stat::FramesPerSecond));
   m_uiViewport.set_text_content("info_dialog/metrics/fps_min/value"_name, framerateMinStr);

   const auto framerateMaxStr = std::format("{:.1f}", StatisticManager::the().max(Stat::FramesPerSecond));
   m_uiViewport.set_text_content("info_dialog/metrics/fps_max/value"_name, framerateMaxStr);

   const auto framerateAvgStr = std::format("{:.1f}", StatisticManager::the().average(Stat::FramesPerSecond));
   m_uiViewport.set_text_content("info_dialog/metrics/fps_avg/value"_name, framerateAvgStr);

   const auto gBufferGpuTimeStr = std::format("{:.2f}ms", StatisticManager::the().value(Stat::GBufferGpuTime));
   m_uiViewport.set_text_content("info_dialog/metrics/gpu_time/value"_name, gBufferGpuTimeStr);

   if (!isFirstFrame) {
      const auto primitiveCountStr = std::format("{}", m_resourceStorage.pipeline_stats().get_int(0));
      m_uiViewport.set_text_content("info_dialog/metrics/triangles/value"_name, primitiveCountStr);
   }

   const auto camPos = m_scene.camera().position();
   const auto positionStr = std::format("{:.2f}, {:.2f}, {:.2f}", camPos.x, camPos.y, camPos.z);
   m_uiViewport.set_text_content("info_dialog/location/position/value"_name, positionStr);

   const auto orientationStr = std::format("{:.2f}, {:.2f}", m_scene.pitch(), m_scene.yaw());
   m_uiViewport.set_text_content("info_dialog/location/orientation/value"_name, orientationStr);
}

void Renderer::on_render()
{
   static bool isFirstFrame = true;

   const auto deltaTime = calculate_frame_duration();

   if (m_mustRecreateSwapchain) {
      auto dim = m_desktopSurface.dimension();
      this->recreate_swapchain(dim.x, dim.y);
   }

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
   }

   m_frameFences[m_frameIndex].await();

   m_updateViewParamsJob.prepare_frame(m_jobGraph, m_frameIndex, deltaTime);
   m_updateUserInterfaceJob.prepare_frame(m_jobGraph, m_frameIndex);

   m_jobGraph.build_semaphores();

   const auto [framebufferIndex, mustRecreate] = GAPI_CHECK(m_swapchain.get_available_framebuffer(
      m_jobGraph.semaphore("job.copy_present_image"_name, "job.acquire_swapchain_image"_name, m_frameIndex)));

   m_jobGraph.execute(RenderingJob::JobName, m_frameIndex, nullptr);

   if (mustRecreate) {
      m_mustRecreateSwapchain = true;
   }

   GAPI_CHECK_STATUS(
      m_device.submit_command_list(m_prePresentCommands[framebufferIndex * render_core::FRAMES_IN_FLIGHT_COUNT + m_frameIndex],
                                   m_jobGraph.wait_semaphores("job.copy_present_image"_name, m_frameIndex),
                                   m_jobGraph.signal_semaphores("job.copy_present_image"_name, m_frameIndex), &m_frameFences[m_frameIndex],
                                   graphics_api::WorkType::Presentation));

   const auto status = m_swapchain.present(m_jobGraph.wait_semaphores("job.present_swapchain_image"_name, m_frameIndex), framebufferIndex);
   if (status == graphics_api::Status::OutOfDateSwapchain) {
      m_mustRecreateSwapchain = true;
   } else {
      GAPI_CHECK_STATUS(status);
   }

   StatisticManager::the().tick();

   m_frameIndex = (m_frameIndex + 1) % render_core::FRAMES_IN_FLIGHT_COUNT;
}

void Renderer::on_close()
{
   m_device.await_all();
}

void Renderer::on_mouse_relative_move(const float dx, const float dy)
{
   m_mouseOffset += glm::vec2{-dx * 0.1f, dy * 0.1f};
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
   return {m_resolution.width, m_resolution.height};
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

   m_jobGraph.set_screen_size({m_resolution.width, m_resolution.height});

   auto& renderingCtx = m_jobGraph.replace_job(RenderingJob::JobName);

   m_renderingJob.build_job(renderingCtx);

   m_jobGraph.rebuild_job(RenderingJob::JobName);

   Renderer::prepare_pre_present_commands(m_device, m_swapchain, m_resourceStorage, m_prePresentCommands);

   m_mustRecreateJobs = false;
}

void Renderer::prepare_pre_present_commands(const graphics_api::Device& device, const graphics_api::Swapchain& swapchain,
                                            render_core::ResourceStorage& resources, std::vector<graphics_api::CommandList>& outCmdLists)
{
   outCmdLists.clear();
   outCmdLists.reserve(swapchain.textures().size() * render_core::FRAMES_IN_FLIGHT_COUNT);

   for (const auto& swapchainTexture : swapchain.textures()) {
      for (const u32 frameIndex : Range(0, render_core::FRAMES_IN_FLIGHT_COUNT)) {
         auto cmdList = GAPI_CHECK(device.create_command_list(graphics_api::WorkType::Graphics | graphics_api::WorkType::Transfer));
         GAPI_CHECK_STATUS(cmdList.begin());

         graphics_api::TextureBarrierInfo inBarrier{};
         inBarrier.texture = &swapchainTexture;
         inBarrier.sourceState = graphics_api::TextureState::Undefined;
         inBarrier.targetState = graphics_api::TextureState::TransferDst;
         inBarrier.baseMipLevel = 0;
         inBarrier.mipLevelCount = 1;
         cmdList.texture_barrier(graphics_api::PipelineStage::Entrypoint, graphics_api::PipelineStage::Transfer, inBarrier);

         cmdList.copy_texture(resources.texture("core.color_out"_name, frameIndex), graphics_api::TextureState::TransferSrc,
                              swapchainTexture, graphics_api::TextureState::TransferDst);

         graphics_api::TextureBarrierInfo outBarrier{};
         outBarrier.texture = &swapchainTexture;
         outBarrier.sourceState = graphics_api::TextureState::TransferDst;
         outBarrier.targetState = graphics_api::TextureState::Present;
         outBarrier.baseMipLevel = 0;
         outBarrier.mipLevelCount = 1;
         cmdList.texture_barrier(graphics_api::PipelineStage::Transfer, graphics_api::PipelineStage::End, outBarrier);

         GAPI_CHECK_STATUS(cmdList.finish());

         outCmdLists.emplace_back(std::move(cmdList));
      }
   }
}

void Renderer::on_resize(const uint32_t width, const uint32_t height)
{
   if (m_resolution.width == width && m_resolution.height == height)
      return;

   this->recreate_swapchain(width, height);
}

void Renderer::recreate_swapchain(const u32 width, const u32 height)
{
   if (m_resolution.width != width || m_resolution.height != height) {
      m_mustRecreateJobs = true;
   }

   const graphics_api::Resolution resolution{width, height};

   m_device.await_all();
   m_swapchain = checkResult(m_device.create_swapchain(m_surface, m_swapchain.color_format(), graphics_api::ColorSpace::sRGB, resolution,
                                                       get_present_mode(), &m_swapchain));

   m_resolution = {width, height};

   if (!m_mustRecreateJobs) {
      Renderer::prepare_pre_present_commands(m_device, m_swapchain, m_resourceStorage, m_prePresentCommands);
   }

   m_mustRecreateSwapchain = false;
}

graphics_api::Device& Renderer::device() const
{
   return m_device;
}

void Renderer::on_config_property_changed(const ConfigProperty property, const Config& config)
{
   m_mustRecreateJobs = true;
   m_renderingJob.set_config(config);

   switch (property) {
   case ConfigProperty::AmbientOcclusion:
      m_uiViewport.set_text_content("info_dialog/features/ao/value"_name, ambient_occlusion_method_to_string(config.ambientOcclusion));
      break;
   case ConfigProperty::Antialiasing:
      m_uiViewport.set_text_content("info_dialog/features/aa/value"_name, antialiasing_method_to_string(config.antialiasing));
      break;
   case ConfigProperty::ShadowCasting:
      m_uiViewport.set_text_content("info_dialog/features/shadows/value"_name, shadow_casting_method_to_string(config.shadowCasting));
      break;
   case ConfigProperty::IsBloomEnabled:
      m_uiViewport.set_text_content("info_dialog/features/bloom/value"_name, config.isBloomEnabled ? "On" : "Off");
      break;
   case ConfigProperty::IsSmoothCameraEnabled:
      m_uiViewport.set_text_content("info_dialog/features/smooth_camera/value"_name, config.isSmoothCameraEnabled ? "On" : "Off");
      break;
   default:
      break;
   }
}

void Renderer::init_config_labels()
{
   const auto& config = m_configManager.config();
   m_uiViewport.set_text_content("info_dialog/features/ao/value"_name, ambient_occlusion_method_to_string(config.ambientOcclusion));
   m_uiViewport.set_text_content("info_dialog/features/aa/value"_name, antialiasing_method_to_string(config.antialiasing));
   m_uiViewport.set_text_content("info_dialog/features/shadows/value"_name, shadow_casting_method_to_string(config.shadowCasting));
   m_uiViewport.set_text_content("info_dialog/features/bloom/value"_name, config.isBloomEnabled ? "On" : "Off");
   m_uiViewport.set_text_content("info_dialog/features/smooth_camera/value"_name, config.isSmoothCameraEnabled ? "On" : "Off");
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

   m_scene.update(m_resolution);

   if (updated) {
      m_scene.update_shadow_maps();
      m_scene.send_view_changed();
   }
}

}// namespace triglav::renderer
