#include "Renderer.h"

#include "node/AmbientOcclusion.h"
#include "node/Geometry.h"
#include "node/PostProcessing.h"
#include "node/Shading.h"
#include "node/ShadowMap.h"
#include "node/UserInterface.h"

#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/Name.hpp"
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

// using triglav::re

namespace triglav::renderer {

constexpr auto g_colorFormat = GAPI_FORMAT(BGRA, sRGB);
constexpr auto g_depthFormat = GAPI_FORMAT(D, UNorm16);
constexpr auto g_sampleCount = SampleCount::Single;

namespace {

graphics_api::Resolution create_viewport_resolution(const graphics_api::Device &device, const uint32_t width,
                                                    const uint32_t height)
{
   graphics_api::Resolution resolution{
           .width  = width,
           .height = height,
   };

   const auto [minResolution, maxResolution] = device.get_surface_resolution_limits();
   resolution.width  = std::clamp(resolution.width, minResolution.width, maxResolution.width);
   resolution.height = std::clamp(resolution.height, minResolution.height, maxResolution.height);

   return resolution;
}

std::vector<graphics_api::Framebuffer> create_framebuffers(const graphics_api::Swapchain &swapchain,
                                                           const graphics_api::RenderTarget &renderTarget)
{
   std::vector<graphics_api::Framebuffer> result{};
   const auto frameCount = swapchain.frame_count();
   for (u32 i = 0; i < frameCount; ++i) {
      result.emplace_back(GAPI_CHECK(renderTarget.create_swapchain_framebuffer(swapchain, i)));
   }
   return result;
}

}// namespace

Renderer::Renderer(const desktop::ISurface &surface, const uint32_t width, const uint32_t height) :
    m_device(checkResult(graphics_api::initialize_device(surface))),
    m_resourceManager(std::make_unique<ResourceManager>(*m_device, m_fontManger)),
    m_scene(*m_resourceManager),
    m_resolution(create_viewport_resolution(*m_device, width, height)),
    m_swapchain(checkResult(
            m_device->create_swapchain(g_colorFormat, graphics_api::ColorSpace::sRGB, m_resolution))),
    m_renderTarget(checkResult(
            graphics_api::RenderTargetBuilder(*m_device)
                    .attachment("output"_name_id,
                                AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                        AttachmentAttribute::StoreImage | AttachmentAttribute::Presentable,
                                g_colorFormat)
                    .attachment("output/zbuffer"_name_id,
                                AttachmentAttribute::Depth | AttachmentAttribute::ClearImage, g_depthFormat)
                    .build())),
    m_framebuffers(create_framebuffers(m_swapchain, m_renderTarget)),
    m_framebufferReadySemaphore(checkResult(m_device->create_semaphore())),
    m_context2D(*m_device, m_renderTarget, *m_resourceManager),
    m_renderGraph(*m_device)
{

   // m_textRenderer.update_resolution(m_resolution);
   m_context2D.update_resolution(m_resolution);

   m_renderGraph.add_semaphore_node("frame_is_ready"_name_id, &m_framebufferReadySemaphore);
   m_renderGraph.emplace_node<node::Geometry>("geometry"_name_id, *m_device, *m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::ShadowMap>("shadow_map"_name_id, *m_device, *m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::AmbientOcclusion>("ambient_occlusion"_name_id, *m_device, *m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::Shading>("shading"_name_id, *m_device, *m_resourceManager, m_scene);
   m_renderGraph.emplace_node<node::UserInterface>("user_interface"_name_id, *m_device, m_resolution, *m_resourceManager);
   m_renderGraph.emplace_node<node::PostProcessing>("post_processing"_name_id, *m_device, *m_resourceManager, m_renderTarget, m_framebuffers);

   m_renderGraph.add_dependency("ambient_occlusion"_name_id, "geometry"_name_id);
   m_renderGraph.add_dependency("shading"_name_id, "shadow_map"_name_id);
   m_renderGraph.add_dependency("shading"_name_id, "ambient_occlusion"_name_id);
   m_renderGraph.add_dependency("post_processing"_name_id, "frame_is_ready"_name_id);
   m_renderGraph.add_dependency("post_processing"_name_id, "shading"_name_id);
   m_renderGraph.add_dependency("post_processing"_name_id, "user_interface"_name_id);

   m_renderGraph.bake("post_processing"_name_id);
   m_renderGraph.update_resolution(m_resolution);

   m_scene.load_level("demo.level"_name);

   auto &ui = m_renderGraph.node<node::UserInterface>("user_interface"_name_id);
   ui.add_label_group("metrics"_name_id, "Metrics");
   ui.add_label_group("features"_name_id, "Features");
   ui.add_label_group("location"_name_id, "Location");

   ui.add_label("metrics"_name_id, "fps"_name_id, "Framerate");
   ui.add_label("metrics"_name_id, "triangles"_name_id, "GBuffer Triangles");
   ui.add_label("metrics"_name_id, "gpu_time"_name_id, "GBuffer Render Time");

   ui.add_label("location"_name_id, "pos"_name_id, "Position");
   ui.add_label("location"_name_id, "orien"_name_id, "Orientation");

   ui.add_label("features"_name_id, "ao"_name_id, "Ambient Occlusion");
   ui.add_label("features"_name_id, "aa"_name_id, "Anti-Aliasing");
   ui.add_label("features"_name_id, "debug_lines"_name_id, "Bounding Boxes");
}

void Renderer::update_debug_info(const float framerate)
{
   static bool isFirstFrame = true;

   auto &ui = m_renderGraph.node<node::UserInterface>("user_interface"_name_id);

   const auto framerateStr = std::format("{}", framerate);
   ui.set_value("fps"_name_id, framerateStr);

   if (not isFirstFrame) {
      const auto gpuTimeStr =
              std::format("{:.2f}ms", m_renderGraph.node<node::Geometry>("geometry"_name_id).gpu_time());
      ui.set_value("gpu_time"_name_id, gpuTimeStr);
   }

   const auto camPos      = m_scene.camera().position();
   const auto positionStr = std::format("{:.2f}, {:.2f}, {:.2f}", camPos.x, camPos.y, camPos.z);
   ui.set_value("pos"_name_id, positionStr);

   const auto orientationStr = std::format("{:.2f}, {:.2f}", m_scene.pitch(), m_scene.yaw());
   ui.set_value("orien"_name_id, orientationStr);

   const auto triangleCountStr = std::format("{}", m_renderGraph.triangle_count("geometry"_name_id));
   ui.set_value("triangles"_name_id, triangleCountStr);

   ui.set_value("ao"_name_id, m_ssaoEnabled ? "Screen-Space" : "Off");
   ui.set_value("aa"_name_id, m_fxaaEnabled ? "FXAA" : "Off");
   ui.set_value("debug_lines"_name_id, m_showDebugLines ? "On" : "Off");

   isFirstFrame = false;
}

void Renderer::on_render()
{
   const auto deltaTime = calculate_frame_duration();
   const auto framerate = calculate_framerate(deltaTime);

   m_renderGraph.await();

   m_renderGraph.set_flag("debug_lines"_name_id, m_showDebugLines);
   m_renderGraph.set_flag("ssao"_name_id, m_ssaoEnabled);
   m_renderGraph.set_flag("fxaa"_name_id, m_fxaaEnabled);
   m_renderGraph.set_flag("hide_ui"_name_id, m_hideUI);
   this->update_debug_info(framerate);

   const auto framebufferIndex = m_swapchain.get_available_framebuffer(m_framebufferReadySemaphore);
   this->update_uniform_data(deltaTime);

   m_renderGraph.node<node::PostProcessing>("post_processing"_name_id).set_index(framebufferIndex);
   m_renderGraph.record_command_lists();

   GAPI_CHECK_STATUS(m_renderGraph.execute());
   checkStatus(m_swapchain.present(*m_renderGraph.target_semaphore(), framebufferIndex));
}

void Renderer::on_close() const
{
   m_renderGraph.await();
   m_device->await_all();
}

void Renderer::on_mouse_relative_move(const float dx, const float dy)
{
   m_scene.update_orientation(-dx * 0.01f, dy * 0.01f);
}

static Renderer::Moving map_direction(const Key key)
{
   switch (key) {
   case Key::W: return Renderer::Moving::Foward;
   case Key::S: return Renderer::Moving::Backwards;
   case Key::A: return Renderer::Moving::Left;
   case Key::D: return Renderer::Moving::Right;
   case Key::Q: return Renderer::Moving::Up;
   case Key::E: return Renderer::Moving::Down;
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
   if (key == Key::Space && m_motion.z == 0.0f) {
      m_motion.z += -12.0f;
   }
}

void Renderer::on_key_released(const triglav::desktop::Key key)
{
   const auto dir = map_direction(key);
   if (m_moveDirection == dir) {
      m_moveDirection = Moving::None;
   }
}

void Renderer::on_mouse_wheel_turn(const float x)
{
   m_distance += x;
   m_distance = std::clamp(m_distance, 1.0f, 100.0f);
}

ResourceManager &Renderer::resource_manager() const
{
   return *m_resourceManager;
}

std::tuple<uint32_t, uint32_t> Renderer::screen_resolution() const
{
   return {m_resolution.width, m_resolution.height};
}

float Renderer::calculate_frame_duration()
{
   static std::chrono::steady_clock::time_point last;

   const auto now  = std::chrono::steady_clock::now();
   const auto diff = now - last;
   last            = now;

   return static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(diff).count()) /
          1000000.0f;
}

float Renderer::calculate_framerate(const float frameDuration)
{
   static float lastResult = 60.0f;
   static float sum        = 0.0f;
   static int count        = 0;

   sum += frameDuration;
   ++count;

   if (sum >= 0.5f) {
      lastResult = static_cast<float>(count) / sum;
      sum        = 0.0f;
      count      = 0;
   }

   return std::ceil(lastResult);
}

glm::vec3 Renderer::moving_direction()
{
   switch (m_moveDirection) {
   case Moving::None: break;
   case Moving::Foward: return m_scene.camera().orientation() * glm::vec3{0.0f, 1.0f, 0.0f};
   case Moving::Backwards: return m_scene.camera().orientation() * glm::vec3{0.0f, -1.0f, 0.0f};
   case Moving::Left: return m_scene.camera().orientation() * glm::vec3{-1.0f, 0.0f, 0.0f};
   case Moving::Right: return m_scene.camera().orientation() * glm::vec3{1.0f, 0.0f, 0.0f};
   case Moving::Up: return glm::vec3{0.0f, 0.0f, -1.0f};
   case Moving::Down: return glm::vec3{0.0f, 0.0f, 1.0f};
   }

   return glm::vec3{0.0f, 0.0f, 0.0f};
}

void Renderer::on_resize(const uint32_t width, const uint32_t height)
{
   if (m_resolution.width == width && m_resolution.height == height)
      return;

   const graphics_api::Resolution resolution{width, height};

   // m_textRenderer.update_resolution(resolution);
   m_context2D.update_resolution(resolution);

   m_device->await_all();

   m_renderGraph.update_resolution(resolution);

   m_framebuffers.clear();

   m_swapchain    = checkResult(m_device->create_swapchain(
           m_swapchain.color_format(), graphics_api::ColorSpace::sRGB, resolution, &m_swapchain));
   m_framebuffers = create_framebuffers(m_swapchain, m_renderTarget);

   m_resolution = {width, height};
}

graphics_api::Device &Renderer::device() const
{
   return *m_device;
}

constexpr auto g_movingSpeed = 10.0f;

void Renderer::update_uniform_data(const float deltaTime)
{
   m_scene.camera().set_position(m_scene.camera().position() + m_motion * deltaTime);

   if (m_moveDirection != Moving::None) {
      glm::vec3 movingDir{this->moving_direction()};
      movingDir.z = 0.0f;
      movingDir   = glm::normalize(movingDir);
      m_scene.camera().set_position(m_scene.camera().position() + movingDir * (g_movingSpeed * deltaTime));
   }

   if (m_scene.camera().position().z >= -4.0f) {
      m_motion = glm::vec3{0.0f};
      glm::vec3 camPos{m_scene.camera().position()};
      camPos.z = -4.0f;
      m_scene.camera().set_position(camPos);
   } else {
      m_motion.z += 30.0f * deltaTime;
   }

   m_scene.update(m_resolution);
}

}// namespace triglav::renderer
