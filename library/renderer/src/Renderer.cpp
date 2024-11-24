#include "Renderer.hpp"

#include "StatisticManager.hpp"
#include "node/AmbientOcclusion.hpp"
#include "node/BindlessGeometry.hpp"
#include "node/Blur.hpp"
#include "node/Geometry.hpp"
#include "node/Particles.hpp"
#include "node/PostProcessing.hpp"
#include "node/ProcessGlyphs.hpp"
#include "node/RayTracedImage.hpp"
#include "node/Shading.hpp"
#include "node/ShadowMap.hpp"
#include "node/SyncBuffers.hpp"
#include "node/UserInterface.hpp"

#include "triglav/Name.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/io/CommandLine.h"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.h"

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
constexpr auto g_depthFormat = GAPI_FORMAT(D, UNorm16);

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

std::vector<graphics_api::Framebuffer> create_framebuffers(const graphics_api::Swapchain& swapchain,
                                                           const graphics_api::RenderTarget& renderTarget)
{
   std::vector<graphics_api::Framebuffer> result{};
   const auto frameCount = swapchain.frame_count();
   for (u32 i = 0; i < frameCount; ++i) {
      result.emplace_back(GAPI_CHECK(renderTarget.create_swapchain_framebuffer(swapchain, i)));
   }
   return result;
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
    m_scene(m_resourceManager),
    m_bindlessScene(m_device, m_resourceManager, m_scene),
    m_resolution(create_viewport_resolution(m_device, m_surface, resolution.width, resolution.height)),
    m_swapchain(
       checkResult(m_device.create_swapchain(m_surface, g_colorFormat, graphics_api::ColorSpace::sRGB, m_resolution, get_present_mode()))),
    m_renderTarget(
       checkResult(graphics_api::RenderTargetBuilder(m_device)
                      .attachment("output"_name,
                                  AttachmentAttribute::Color | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage |
                                     AttachmentAttribute::Presentable,
                                  g_colorFormat)
                      .attachment("output/zbuffer"_name, AttachmentAttribute::Depth | AttachmentAttribute::ClearImage, g_depthFormat)
                      .build())),
    m_framebuffers(create_framebuffers(m_swapchain, m_renderTarget)),
    m_glyphCache(m_device, m_resourceManager),
    m_context2D(m_device, m_renderTarget, m_resourceManager),
    m_renderGraph(m_device),
    m_infoDialog(m_uiViewport, m_resourceManager, m_glyphCache),
    m_rayTracingScene((m_device.enabled_features() & DeviceFeature::RayTracing)
                         ? std::make_optional<RayTracingScene>(m_device, m_resourceManager, m_scene)
                         : std::nullopt)
{
   m_context2D.update_resolution(m_resolution);

   m_renderGraph.add_external_node("frame_is_ready"_name);
   // m_renderGraph.emplace_node<node::Geometry>("geometry"_name, m_device, m_resourceManager, m_scene, m_renderGraph);
   m_renderGraph.emplace_node<node::BindlessGeometry>("geometry"_name, m_device, m_bindlessScene, m_resourceManager);
   m_renderGraph.emplace_node<node::ShadowMap>("shadow_map"_name, m_device, m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::AmbientOcclusion>("ambient_occlusion"_name, m_device, m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::Shading>("shading"_name, m_device, m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::UserInterface>("user_interface"_name, m_device, m_resourceManager, m_uiViewport, m_glyphCache);
   m_renderGraph.emplace_node<node::PostProcessing>("post_processing"_name, m_device, m_resourceManager, m_renderTarget, m_framebuffers);
   m_renderGraph.emplace_node<node::Particles>("particles"_name, m_device, m_resourceManager, m_renderGraph);
   m_renderGraph.emplace_node<node::SyncBuffers>("sync_buffers"_name, m_scene);
   m_renderGraph.emplace_node<node::ProcessGlyphs>("process_glyphs"_name, m_device, m_resourceManager, m_glyphCache, m_uiViewport);
   m_renderGraph.emplace_node<node::Blur>("blur_bloom"_name, m_device, m_resourceManager, "shading"_name, "bloom"_name, false);
   m_renderGraph.emplace_node<node::Blur>("blur_ao"_name, m_device, m_resourceManager, "ambient_occlusion"_name, "ao"_name, true);

   if (m_device.enabled_features() & DeviceFeature::RayTracing) {
      m_renderGraph.emplace_node<node::RayTracedImage>("ray_tracing"_name, m_device, *m_rayTracingScene, m_scene);
   }

   m_renderGraph.add_interframe_dependency("particles"_name, "particles"_name);
   m_renderGraph.add_interframe_dependency("geometry"_name, "shading"_name);

   m_renderGraph.add_dependency("geometry"_name, "sync_buffers"_name);
   m_renderGraph.add_dependency("user_interface"_name, "process_glyphs"_name);
   m_renderGraph.add_dependency("ambient_occlusion"_name, "geometry"_name);
   m_renderGraph.add_dependency("blur_ao"_name, "ambient_occlusion"_name);
   m_renderGraph.add_dependency("shading"_name, "shadow_map"_name);
   m_renderGraph.add_dependency("shading"_name, "blur_ao"_name);
   m_renderGraph.add_dependency("shading"_name, "particles"_name);
   m_renderGraph.add_dependency("blur_bloom"_name, "shading"_name);
   m_renderGraph.add_dependency("post_processing"_name, "frame_is_ready"_name);
   m_renderGraph.add_dependency("post_processing"_name, "user_interface"_name);
   m_renderGraph.add_dependency("post_processing"_name, "blur_bloom"_name);

   if (m_device.enabled_features() & graphics_api::DeviceFeature::RayTracing) {
      m_renderGraph.add_dependency("blur_ao"_name, "ray_tracing"_name);
   }

   m_renderGraph.bake("post_processing"_name);
   m_renderGraph.update_resolution(m_resolution);

   m_infoDialog.initialize();
   m_scene.load_level("demo.level"_rc);

   StatisticManager::the().initialize();

   m_scene.update_shadow_maps();
}

void Renderer::update_debug_info()
{
   const auto framerateStr = std::format("{:.1f}", StatisticManager::the().value(Stat::FramesPerSecond));
   m_uiViewport.set_text_content("info_dialog/metrics/fps/value"_name, framerateStr);

   const auto framerateMinStr = std::format("{:.1f}", StatisticManager::the().min(Stat::FramesPerSecond));
   m_uiViewport.set_text_content("info_dialog/metrics/fps_min/value"_name, framerateMinStr);

   const auto framerateMaxStr = std::format("{:.1f}", StatisticManager::the().max(Stat::FramesPerSecond));
   m_uiViewport.set_text_content("info_dialog/metrics/fps_max/value"_name, framerateMaxStr);

   const auto framerateAvgStr = std::format("{:.1f}", StatisticManager::the().average(Stat::FramesPerSecond));
   m_uiViewport.set_text_content("info_dialog/metrics/fps_avg/value"_name, framerateAvgStr);

   const auto gBufferTriangleCountStr = std::format("{}", m_renderGraph.triangle_count("geometry"_name));
   m_uiViewport.set_text_content("info_dialog/metrics/gbuffer_triangles/value"_name, gBufferTriangleCountStr);
   const auto shadingTriangleCountStr = std::format("{}", m_renderGraph.triangle_count("shading"_name));
   m_uiViewport.set_text_content("info_dialog/metrics/shading_triangles/value"_name, shadingTriangleCountStr);

   const auto gBufferGpuTimeStr = std::format("{:.2f}ms", StatisticManager::the().value(Stat::GBufferGpuTime));
   m_uiViewport.set_text_content("info_dialog/metrics/gbuffer_gpu_time/value"_name, gBufferGpuTimeStr);
   const auto shadingGpuTimeStr = std::format("{:.2f}ms", StatisticManager::the().value(Stat::ShadingGpuTime));
   m_uiViewport.set_text_content("info_dialog/metrics/shading_gpu_time/value"_name, shadingGpuTimeStr);

   if (this->is_any_ray_tracing_feature_enabled()) {
      const auto rtGpuTimeStr = std::format("{:.2f}ms", StatisticManager::the().value(Stat::RayTracingGpuTime));
      m_uiViewport.set_text_content("info_dialog/metrics/ray_tracing_gpu_time/value"_name, rtGpuTimeStr);
   } else {
      m_uiViewport.set_text_content("info_dialog/metrics/ray_tracing_gpu_time/value"_name, "Disabled");
   }

   const auto camPos = m_scene.camera().position();
   const auto positionStr = std::format("{:.2f}, {:.2f}, {:.2f}", camPos.x, camPos.y, camPos.z);
   m_uiViewport.set_text_content("info_dialog/location/position/value"_name, positionStr);

   const auto orientationStr = std::format("{:.2f}, {:.2f}", m_scene.pitch(), m_scene.yaw());
   m_uiViewport.set_text_content("info_dialog/location/orientation/value"_name, orientationStr);

   m_uiViewport.set_text_content("info_dialog/features/ao/value"_name, ambient_occlusion_method_to_string(m_aoMethod));
   m_uiViewport.set_text_content("info_dialog/features/aa/value"_name, m_fxaaEnabled ? "FXAA" : "Off");
   m_uiViewport.set_text_content("info_dialog/features/shadows/value"_name, m_rayTracedShadows ? "Ray Traced" : "Cascaded Shadow Maps");
   m_uiViewport.set_text_content("info_dialog/features/bloom/value"_name, m_bloomEnabled ? "On" : "Off");
   m_uiViewport.set_text_content("info_dialog/features/debug_lines/value"_name, m_showDebugLines ? "On" : "Off");
   m_uiViewport.set_text_content("info_dialog/features/smooth_camera/value"_name, m_smoothCamera ? "On" : "Off");
}

void Renderer::on_render()
{
   static bool isFirstFrame = true;

   const auto deltaTime = calculate_frame_duration();

   if (not isFirstFrame) {
      StatisticManager::the().push_accumulated(Stat::FramesPerSecond, 1.0f / deltaTime);
      // StatisticManager::the().push_accumulated(Stat::GBufferGpuTime, m_renderGraph.node<node::Geometry>("geometry"_name).gpu_time());
      StatisticManager::the().push_accumulated(Stat::ShadingGpuTime, m_renderGraph.node<node::Shading>("shading"_name).gpu_time());
      if (this->is_any_ray_tracing_feature_enabled()) {
         StatisticManager::the().push_accumulated(Stat::RayTracingGpuTime,
                                                  m_renderGraph.node<node::RayTracedImage>("ray_tracing"_name).gpu_time());
      }
   } else {
      isFirstFrame = false;
   }

   if (m_mustRecreateSwapchain) {
      auto dim = m_desktopSurface.dimension();
      this->recreate_swapchain(dim.width, dim.height);
      m_mustRecreateSwapchain = false;
   }

   m_renderGraph.change_active_frame();
   m_renderGraph.await();

   auto& frameReadySemaphore = m_renderGraph.semaphore("frame_is_ready"_name, "post_processing"_name);
   const auto [framebufferIndex, mustRecreate] = GAPI_CHECK(m_swapchain.get_available_framebuffer(frameReadySemaphore));
   if (mustRecreate) {
      m_mustRecreateSwapchain = true;
   }

   m_renderGraph.set_flag("debug_lines"_name, m_showDebugLines);
   m_renderGraph.set_option("ao_method"_name, m_aoMethod);
   m_renderGraph.set_flag("fxaa"_name, m_fxaaEnabled);
   m_renderGraph.set_flag("bloom"_name, m_bloomEnabled);
   m_renderGraph.set_flag("hide_ui"_name, m_hideUI);
   m_renderGraph.set_flag("ray_traced_shadows"_name, m_rayTracedShadows);
   this->update_debug_info();

   this->update_uniform_data(deltaTime);

   m_renderGraph.node<node::Particles>("particles"_name).set_delta_time(deltaTime);
   m_renderGraph.node<node::PostProcessing>("post_processing"_name).set_index(framebufferIndex);
   m_renderGraph.record_command_lists();

   GAPI_CHECK_STATUS(m_renderGraph.execute());
   auto status = m_swapchain.present(m_renderGraph.target_semaphore(), framebufferIndex);
   if (status == graphics_api::Status::OutOfDateSwapchain) {
      m_mustRecreateSwapchain = true;
   } else {
      GAPI_CHECK_STATUS(status);
   }

   StatisticManager::the().tick();
}

void Renderer::on_close()
{
   m_renderGraph.await();
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
      switch (m_aoMethod) {
      case AmbientOcclusionMethod::None:
         m_renderGraph.node<node::Blur>("blur_ao"_name).set_source_texture("ambient_occlusion"_name, "ao"_name);
         m_aoMethod = AmbientOcclusionMethod::ScreenSpace;
         break;
      case AmbientOcclusionMethod::ScreenSpace:
         if (m_device.enabled_features() & DeviceFeature::RayTracing) {
            m_renderGraph.node<node::Blur>("blur_ao"_name).set_source_texture("ray_tracing"_name, "ao"_name);
            m_aoMethod = AmbientOcclusionMethod::RayTraced;
         } else {
            m_aoMethod = AmbientOcclusionMethod::None;
         }
         break;
      case AmbientOcclusionMethod::RayTraced:
         m_aoMethod = AmbientOcclusionMethod::None;
         break;
      }
   }
   if (key == Key::F5) {
      m_fxaaEnabled = not m_fxaaEnabled;
   }
   if (key == Key::F6) {
      m_hideUI = not m_hideUI;
   }
   if (key == Key::F7) {
      m_bloomEnabled = not m_bloomEnabled;
   }
   if (key == Key::F8) {
      m_smoothCamera = not m_smoothCamera;
   }
   if (key == Key::F9) {
      if (m_device.enabled_features() & DeviceFeature::RayTracing) {
         m_rayTracedShadows = not m_rayTracedShadows;
      }
   }
   if (key == Key::Space && m_motion.z == 0.0f) {
      m_motion.z += -16.0f;
   }
}

void Renderer::on_key_released(const Key key)
{
   const auto dir = map_direction(key);
   if (m_moveDirection == dir) {
      m_moveDirection = Moving::None;
   }
}

void Renderer::on_mouse_wheel_turn(const float x) {}

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

void Renderer::on_resize(const uint32_t width, const uint32_t height)
{
   if (m_resolution.width == width && m_resolution.height == height)
      return;

   this->recreate_swapchain(width, height);
}

void Renderer::recreate_swapchain(const u32 width, const u32 height)
{
   const graphics_api::Resolution resolution{width, height};

   m_device.await_all();

   m_context2D.update_resolution(resolution);
   m_renderGraph.update_resolution(resolution);

   m_framebuffers.clear();

   m_swapchain = checkResult(m_device.create_swapchain(m_surface, m_swapchain.color_format(), graphics_api::ColorSpace::sRGB, resolution,
                                                       get_present_mode(), &m_swapchain));
   m_framebuffers = create_framebuffers(m_swapchain, m_renderTarget);

   m_resolution = {width, height};
}

graphics_api::Device& Renderer::device() const
{
   return m_device;
}
bool Renderer::is_any_ray_tracing_feature_enabled() const
{
   if (m_device.enabled_features() & DeviceFeature::RayTracing) {
      return m_rayTracedShadows || m_aoMethod == AmbientOcclusionMethod::RayTraced;
   }
   return false;
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

   if (m_scene.camera().position().z >= -5.0f) {
      m_motion = glm::vec3{0.0f};
      glm::vec3 camPos{m_scene.camera().position()};
      camPos.z = -5.0f;
      m_scene.camera().set_position(camPos);
      updated = true;
   } else {
      m_motion.z += 30.0f * deltaTime;
   }

   if (m_smoothCamera) {
      m_scene.update_orientation(m_mouseOffset.x * deltaTime, m_mouseOffset.y * deltaTime);
      m_mouseOffset += m_mouseOffset * (static_cast<float>(pow(0.5f, 50.0f * deltaTime)) - 1.0f);
   } else {
      m_scene.update_orientation(0.05f * m_mouseOffset.x, 0.05f * m_mouseOffset.y);
      m_mouseOffset = {0.0f, 0.0f};
   }

   m_scene.update(m_resolution);

   if (updated) {
      m_scene.update_shadow_maps();
   }
}

}// namespace triglav::renderer
