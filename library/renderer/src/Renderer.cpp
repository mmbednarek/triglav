#include "Renderer.h"

#include "StatisticManager.h"
#include "node/AmbientOcclusion.h"
#include "node/Downsample.h"
#include "node/Geometry.h"
#include "node/Particles.h"
#include "node/PostProcessing.h"
#include "node/ProcessGlyphs.h"
#include "node/Shading.h"
#include "node/ShadowMap.h"
#include "node/SyncBuffers.h"
#include "node/UserInterface.h"

#include "triglav/Name.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/io/CommandLine.h"
#include "triglav/render_core/GlyphAtlas.h"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>

using triglav::ResourceType;
using triglav::desktop::Key;
using triglav::graphics_api::AttachmentAttribute;
using triglav::graphics_api::SampleCount;
using triglav::render_core::checkResult;
using triglav::render_core::checkStatus;
using triglav::resource::ResourceManager;
using namespace triglav::name_literals;

namespace triglav::renderer {

constexpr auto g_colorFormat = GAPI_FORMAT(BGRA, sRGB);
constexpr auto g_depthFormat = GAPI_FORMAT(D, UNorm16);
constexpr auto g_sampleCount = SampleCount::Single;

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
   auto presentModeStr = io::CommandLine::the().arg("presentMode"_name);
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
                   resource::ResourceManager& resourceManager, const graphics_api::Resolution& resolution) :
    m_desktopSurface(desktopSurface),
    m_surface(surface),
    m_device(device),
    m_resourceManager(resourceManager),
    m_scene(m_resourceManager),
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
    m_infoDialog(m_uiViewport, m_resourceManager, m_glyphCache)
{
   m_context2D.update_resolution(m_resolution);

   m_renderGraph.add_external_node("frame_is_ready"_name);
   m_renderGraph.emplace_node<node::Geometry>("geometry"_name, m_device, m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::ShadowMap>("shadow_map"_name, m_device, m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::AmbientOcclusion>("ambient_occlusion"_name, m_device, m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::Shading>("shading"_name, m_device, m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::UserInterface>("user_interface"_name, m_device, m_resourceManager, m_uiViewport, m_glyphCache);
   m_renderGraph.emplace_node<node::PostProcessing>("post_processing"_name, m_device, m_resourceManager, m_renderTarget, m_framebuffers);
   m_renderGraph.emplace_node<node::Downsample>("downsample_bloom"_name, m_device, "shading"_name, "shading"_name, "bloom"_name);
   m_renderGraph.emplace_node<node::Particles>("particles"_name, m_device, m_resourceManager, m_renderGraph);
   m_renderGraph.emplace_node<node::SyncBuffers>("sync_buffers"_name, m_scene);
   m_renderGraph.emplace_node<node::ProcessGlyphs>("process_glyphs"_name, m_device, m_resourceManager, m_glyphCache, m_uiViewport);

   m_renderGraph.add_interframe_dependency("particles"_name, "particles"_name);

   m_renderGraph.add_dependency("geometry"_name, "sync_buffers"_name);
   m_renderGraph.add_dependency("user_interface"_name, "process_glyphs"_name);
   m_renderGraph.add_dependency("ambient_occlusion"_name, "geometry"_name);
   m_renderGraph.add_dependency("shading"_name, "shadow_map"_name);
   m_renderGraph.add_dependency("shading"_name, "ambient_occlusion"_name);
   m_renderGraph.add_dependency("shading"_name, "particles"_name);
   m_renderGraph.add_dependency("downsample_bloom"_name, "shading"_name);
   m_renderGraph.add_dependency("post_processing"_name, "frame_is_ready"_name);
   m_renderGraph.add_dependency("post_processing"_name, "user_interface"_name);
   m_renderGraph.add_dependency("post_processing"_name, "downsample_bloom"_name);

   m_renderGraph.bake("post_processing"_name);
   m_renderGraph.update_resolution(m_resolution);

   m_infoDialog.initialize();
   m_scene.load_level("demo.level"_rc);

   StatisticManager::the().initialize();
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

   const auto camPos = m_scene.camera().position();
   const auto positionStr = std::format("{:.2f}, {:.2f}, {:.2f}", camPos.x, camPos.y, camPos.z);
   m_uiViewport.set_text_content("info_dialog/location/position/value"_name, positionStr);

   const auto orientationStr = std::format("{:.2f}, {:.2f}", m_scene.pitch(), m_scene.yaw());
   m_uiViewport.set_text_content("info_dialog/location/orientation/value"_name, orientationStr);

   m_uiViewport.set_text_content("info_dialog/features/ao/value"_name, m_ssaoEnabled ? "Screen-Space" : "Off");
   m_uiViewport.set_text_content("info_dialog/features/aa/value"_name, m_fxaaEnabled ? "FXAA" : "Off");
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
      StatisticManager::the().push_accumulated(Stat::GBufferGpuTime, m_renderGraph.node<node::Geometry>("geometry"_name).gpu_time());
      StatisticManager::the().push_accumulated(Stat::ShadingGpuTime, m_renderGraph.node<node::Shading>("shading"_name).gpu_time());
   } else {
      isFirstFrame = false;
   }

   m_renderGraph.change_active_frame();
   m_renderGraph.await();

   auto& frameReadySemaphore = m_renderGraph.semaphore("frame_is_ready"_name, "post_processing"_name);
   const auto framebufferIndex = m_swapchain.get_available_framebuffer(frameReadySemaphore);
   if (not framebufferIndex.has_value()) {
      if (framebufferIndex.error() == graphics_api::Status::OutOfDateSwapchain) {
         auto dim = m_desktopSurface.dimension();
         this->recreate_swapchain(dim.width, dim.height);
         return;
      } else {
         GAPI_CHECK_STATUS(framebufferIndex.error());
      }
   }

   m_renderGraph.set_flag("debug_lines"_name, m_showDebugLines);
   m_renderGraph.set_flag("ssao"_name, m_ssaoEnabled);
   m_renderGraph.set_flag("fxaa"_name, m_fxaaEnabled);
   m_renderGraph.set_flag("bloom"_name, m_bloomEnabled);
   m_renderGraph.set_flag("hide_ui"_name, m_hideUI);
   this->update_debug_info();

   this->update_uniform_data(deltaTime);

   m_renderGraph.node<node::Particles>("particles"_name).set_delta_time(deltaTime);
   m_renderGraph.node<node::PostProcessing>("post_processing"_name).set_index(*framebufferIndex);
   m_renderGraph.record_command_lists();

   GAPI_CHECK_STATUS(m_renderGraph.execute());
   auto status = m_swapchain.present(m_renderGraph.target_semaphore(), *framebufferIndex);
   if (status == graphics_api::Status::OutOfDateSwapchain) {
      auto dim = m_desktopSurface.dimension();
      this->recreate_swapchain(dim.width, dim.height);
      return;
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
      return Renderer::Moving::Foward;
   case Key::S:
      return Renderer::Moving::Backwards;
   case Key::A:
      return Renderer::Moving::Left;
   case Key::D:
      return Renderer::Moving::Right;
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
      m_ssaoEnabled = not m_ssaoEnabled;
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
   if (key == Key::Space && m_motion.z == 0.0f) {
      m_motion.z += -32.0f;
   }
}

void Renderer::on_key_released(const triglav::desktop::Key key)
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
   case Moving::Foward:
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

constexpr auto g_movingSpeed = 10.0f;

void Renderer::update_uniform_data(const float deltaTime)
{
   m_scene.camera().set_position(m_scene.camera().position() + m_motion * deltaTime);

   if (m_moveDirection != Moving::None) {
      glm::vec3 movingDir{this->moving_direction()};
      movingDir.z = 0.0f;
      movingDir = glm::normalize(movingDir);
      m_scene.camera().set_position(m_scene.camera().position() + movingDir * (g_movingSpeed * deltaTime));
   }

   if (m_scene.camera().position().z >= -5.0f) {
      m_motion = glm::vec3{0.0f};
      glm::vec3 camPos{m_scene.camera().position()};
      camPos.z = -5.0f;
      m_scene.camera().set_position(camPos);
   } else {
      m_motion.z += 30.0f * deltaTime;
   }

   if (m_smoothCamera) {
      m_scene.update_orientation(m_mouseOffset.x * deltaTime, m_mouseOffset.y * deltaTime);
      m_mouseOffset += m_mouseOffset * (pow(0.5f, 50.0f * deltaTime) - 1.0f);
   } else {
      m_scene.update_orientation(0.1f * m_mouseOffset.x, 0.1f * m_mouseOffset.y);
      m_mouseOffset = {0.0f, 0.0f};
   }

   m_scene.update(m_resolution);
}

}// namespace triglav::renderer
