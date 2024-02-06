#include "Renderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>

#include "geometry/DebugMesh.h"
#include "geometry/Mesh.h"
#include "graphics_api/PipelineBuilder.h"

#include "AssetMap.hpp"
#include "Core.h"
#include "GlyphAtlas.h"
#include "Name.hpp"
#include "ResourceManager.h"

namespace renderer {

constexpr auto g_colorFormat = GAPI_FORMAT(BGRA, sRGB);
constexpr auto g_depthFormat = GAPI_FORMAT(D, Float32);
constexpr auto g_sampleCount = graphics_api::SampleCount::Single;

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

std::unique_ptr<ResourceManager> create_resource_manager(graphics_api::Device &device,
                                                         font::FontManger &fontManger)
{
   auto manager = std::make_unique<ResourceManager>(device, fontManger);

   for (const auto &[name, path] : g_assetMap) {
      manager->load_asset(name, path);
   }

   auto sphere = geometry::create_sphere(40, 20, 4.0f);
   sphere.set_material(0, "gold");
   sphere.triangulate();
   sphere.recalculate_tangents();
   auto gpuSphere = sphere.upload_to_device(device);

   manager->add_mesh_and_model("mdl:sphere"_name, gpuSphere, sphere.calculate_bouding_box());
   manager->add_material("mat:bark"_name, Material{
                                                  "tex:bark"_name,
                                                  "tex:bark/normal"_name,
                                                  {true, 1.0f, 0.0f}
   });
   manager->add_material("mat:leaves"_name, Material{
                                                    "tex:leaves"_name,
                                                    "tex:grass/normal"_name,
                                                    {false, 1.0f, 0.0f}
   });
   manager->add_material("mat:gold"_name, Material{
                                                  "tex:gold"_name,
                                                  "tex:gold/normal"_name,
                                                  {true, 0.1f, 0.5f}
   });
   manager->add_material("mat:grass"_name, Material{
                                                   "tex:grass"_name,
                                                   "tex:grass/normal"_name,
                                                   {true, 1.0f, 0.0f}
   });
   manager->add_material("mat:pine"_name, Material{
                                                  "tex:pine"_name,
                                                  "tex:pine/normal"_name,
                                                  {true, 1.0f, 0.0f}
   });
   manager->add_material("mat:quartz"_name, Material{
                                                    "tex:quartz"_name,
                                                    "tex:quartz/normal"_name,
                                                    {false, 0.2f, 0.1f}
   });

   return manager;
}

std::vector<font::Rune> make_runes()
{
   std::vector<font::Rune> runes{};
   for (font::Rune ch = 'A'; ch <= 'Z'; ++ch) {
      runes.emplace_back(ch);
   }
   for (font::Rune ch = 'a'; ch <= 'z'; ++ch) {
      runes.emplace_back(ch);
   }
   for (font::Rune ch = '0'; ch <= '9'; ++ch) {
      runes.emplace_back(ch);
   }
   runes.emplace_back('.');
   runes.emplace_back(':');
   runes.emplace_back('-');
   runes.emplace_back(',');
   runes.emplace_back('(');
   runes.emplace_back(')');
   runes.emplace_back(' ');
   runes.emplace_back(281);
   return runes;
}

graphics_api::TextureRenderTarget create_model_render_target(const graphics_api::Device &device,
                                                             const graphics_api::Resolution resolution)
{
   using graphics_api::AttachmentLifetime;
   using graphics_api::AttachmentType;
   using graphics_api::SampleCount;

   auto renderTarget = checkResult(device.create_texture_render_target(resolution));
   // Color
   renderTarget.add_attachment(AttachmentType::Color, AttachmentLifetime::ClearPreserve,
                               GAPI_FORMAT(RGBA, Float16), SampleCount::Single);
   // Position
   renderTarget.add_attachment(AttachmentType::Color, AttachmentLifetime::ClearPreserve,
                               GAPI_FORMAT(RGBA, Float16), SampleCount::Single);
   // Normaal
   renderTarget.add_attachment(AttachmentType::Color, AttachmentLifetime::ClearPreserve,
                               GAPI_FORMAT(RGBA, Float16), SampleCount::Single);
   // Depth
   renderTarget.add_attachment(AttachmentType::Depth, AttachmentLifetime::ClearPreserve, g_depthFormat,
                               SampleCount::Single);
   return renderTarget;
}

graphics_api::TextureRenderTarget create_ao_render_target(const graphics_api::Device &device,
                                                          const graphics_api::Resolution resolution)
{
   using graphics_api::AttachmentLifetime;
   using graphics_api::AttachmentType;
   using graphics_api::SampleCount;

   auto renderTarget = checkResult(device.create_texture_render_target(resolution));
   renderTarget.add_attachment(AttachmentType::Color, AttachmentLifetime::ClearPreserve,
                               GAPI_FORMAT(R, Float16), SampleCount::Single);
   return renderTarget;
}

graphics_api::TextureRenderTarget create_shading_render_target(const graphics_api::Device &device,
                                                               const graphics_api::Resolution resolution)
{
   using graphics_api::AttachmentLifetime;
   using graphics_api::AttachmentType;
   using graphics_api::SampleCount;

   auto renderTarget = checkResult(device.create_texture_render_target(resolution));
   renderTarget.add_attachment(AttachmentType::Color, AttachmentLifetime::ClearPreserve,
                               GAPI_FORMAT(RGBA, Float16), SampleCount::Single);
   return renderTarget;
}

auto g_runes{make_runes()};

}// namespace

Renderer::Renderer(const graphics_api::Surface &surface, const uint32_t width, const uint32_t height) :
    m_device(checkResult(graphics_api::initialize_device(surface))),
    m_resourceManager(create_resource_manager(*m_device, m_fontManger)),
    m_resolution(create_viewport_resolution(*m_device, width, height)),
    m_swapchain(checkResult(m_device->create_swapchain(g_colorFormat, graphics_api::ColorSpace::sRGB,
                                                       g_depthFormat, g_sampleCount, m_resolution))),
    m_renderPass(checkResult(m_device->create_render_pass(m_swapchain))),
    m_framebuffers(checkResult(m_swapchain.create_framebuffers(m_renderPass))),
    m_framebufferReadySemaphore(checkResult(m_device->create_semaphore())),
    m_renderFinishedSemaphore(checkResult(m_device->create_semaphore())),
    m_inFlightFence(checkResult(m_device->create_fence())),
    m_commandList(checkResult(m_device->create_command_list())),
    m_modelRenderTarget(create_model_render_target(*m_device, m_resolution)),
    m_modelRenderPass(checkResult(m_device->create_render_pass(m_modelRenderTarget))),
    m_modelColorTexture(checkResult(m_device->create_texture(GAPI_FORMAT(RGBA, Float16), m_resolution,
                                                             graphics_api::TextureType::ColorAttachment))),
    m_modelPositionTexture(checkResult(m_device->create_texture(GAPI_FORMAT(RGBA, Float16), m_resolution,
                                                                graphics_api::TextureType::ColorAttachment))),
    m_modelNormalTexture(checkResult(m_device->create_texture(GAPI_FORMAT(RGBA, Float16), m_resolution,
                                                              graphics_api::TextureType::ColorAttachment))),
    m_modelDepthTexture(checkResult(m_device->create_texture(g_depthFormat, m_resolution,
                                                             graphics_api::TextureType::SampledDepthBuffer))),
    m_modelFramebuffer(checkResult(m_modelRenderTarget.create_framebuffer(
            m_modelRenderPass, m_modelColorTexture, m_modelPositionTexture, m_modelNormalTexture,
            m_modelDepthTexture))),
    m_ambientOcclusionRenderTarget(create_ao_render_target(*m_device, m_resolution)),
    m_ambientOcclusionRenderPass(checkResult(m_device->create_render_pass(m_ambientOcclusionRenderTarget))),
    m_ambientOcclusionTexture(checkResult(m_device->create_texture(
            GAPI_FORMAT(R, Float16), m_resolution, graphics_api::TextureType::ColorAttachment))),
    m_ambientOcclusionFramebuffer(checkResult(m_ambientOcclusionRenderTarget.create_framebuffer(
            m_ambientOcclusionRenderPass, m_ambientOcclusionTexture))),
    m_shadingRenderTarget(create_shading_render_target(*m_device, m_resolution)),
    m_shadingRenderPass(checkResult(m_device->create_render_pass(m_shadingRenderTarget))),
    m_shadingColorTexture(checkResult(m_device->create_texture(GAPI_FORMAT(RGBA, Float16), m_resolution,
                                                               graphics_api::TextureType::ColorAttachment))),
    m_shadingFramebuffer(checkResult(
            m_shadingRenderTarget.create_framebuffer(m_shadingRenderPass, m_shadingColorTexture))),
    m_context3D(*m_device, m_modelRenderPass, *m_resourceManager),
    m_groundRenderer(*m_device, m_modelRenderPass, *m_resourceManager),
    m_context2D(*m_device, m_renderPass, *m_resourceManager),
    m_shadowMap(*m_device, *m_resourceManager),
    m_debugLinesRenderer(*m_device, m_modelRenderPass, *m_resourceManager),
    m_rectangleRenderer(*m_device, m_renderPass, *m_resourceManager),
    m_rectangle(m_rectangleRenderer.create_rectangle(glm::vec4{5.0f, 5.0f, 480.0f, 250.0f})),
    m_ambientOcclusionRenderer(*m_device, m_ambientOcclusionRenderPass, *m_resourceManager,
                               m_modelPositionTexture, m_modelNormalTexture,
                               m_resourceManager->texture("tex:noise"_name)),
    m_shadingRenderer(*m_device, m_shadingRenderPass, *m_resourceManager, m_modelColorTexture,
                      m_modelPositionTexture, m_modelNormalTexture, m_ambientOcclusionTexture,
                      m_shadowMap.depth_texture()),
    m_postProcessingRenderer(*m_device, m_renderPass, *m_resourceManager, m_shadingColorTexture, m_modelDepthTexture),
    m_scene(*this, m_context3D, m_shadowMap, m_debugLinesRenderer, *m_resourceManager),
    m_skyBox(*this),
    m_glyphAtlasBold(*m_device, m_resourceManager->typeface("tfc:cantarell/bold"_name), g_runes, 24, 500,
                     500),
    m_glyphAtlas(*m_device, m_resourceManager->typeface("tfc:cantarell"_name), g_runes, 24, 500, 500),
    m_sprite(m_context2D.create_sprite_from_texture(m_shadowMap.depth_texture())),
    // m_sprite(m_context2D.create_sprite_from_texture(m_glyphAtlas.texture())),
    m_textRenderer(*m_device, m_renderPass, *m_resourceManager),
    m_titleLabel(m_textRenderer.create_text_object(m_glyphAtlas, "Triglav Engine Demo")),
    m_framerateLabel(m_textRenderer.create_text_object(m_glyphAtlasBold, "Framerate")),
    m_framerateValue(m_textRenderer.create_text_object(m_glyphAtlas, "0")),
    m_positionLabel(m_textRenderer.create_text_object(m_glyphAtlasBold, "Position")),
    m_positionValue(m_textRenderer.create_text_object(m_glyphAtlas, "0, 0, 0")),
    m_orientationLabel(m_textRenderer.create_text_object(m_glyphAtlasBold, "Orientation")),
    m_orientationValue(m_textRenderer.create_text_object(m_glyphAtlas, "0, 0")),
    m_triangleCountLabel(m_textRenderer.create_text_object(m_glyphAtlasBold, "Triangle Count")),
    m_triangleCountValue(m_textRenderer.create_text_object(m_glyphAtlas, "0")),
    m_aoLabel(m_textRenderer.create_text_object(m_glyphAtlasBold, "Ambient Occlusion")),
    m_aoValue(m_textRenderer.create_text_object(m_glyphAtlas, "Screen-Space")),
    m_aaLabel(m_textRenderer.create_text_object(m_glyphAtlasBold, "Anti-Aliasing")),
    m_aaValue(m_textRenderer.create_text_object(m_glyphAtlas, "Off"))
{

   m_scene.add_object(SceneObject{
           .model{"mdl:hall"_name},
           .position{0, 0, 0},
           .rotation{1, 0, 0, 0},
           .scale{10, 10, 10},
   });

   m_scene.add_object(SceneObject{
           .model{"mdl:teapot"_name},
           .position{12.0f, -12.0f, 0},
           .rotation{glm::vec3{0, 0, glm::radians(0.0f)}},
           .scale{1.2, 1.2, 1.2},
   });

   m_scene.add_object(SceneObject{
           .model{"mdl:pine"_name},
           .position{12.0f, -20.0f, 0},
           .rotation{glm::vec3{0, 0, glm::radians(0.0f)}},
           .scale{30.0, 30.0, 30.0},
   });

   m_scene.add_object(SceneObject{
           .model{"mdl:column"_name},
           .position{0.0f, -32.0f, 0.0f},
           .rotation{1, 0, 0, 0},
           .scale{10, 10, 10},
   });

   m_scene.add_object(SceneObject{
           .model{"mdl:column"_name},
           .position{0.0f, -16.0f, 0.0f},
           .rotation{1, 0, 0, 0},
           .scale{10, 10, 10},
   });

   m_scene.add_object(SceneObject{
           .model{"mdl:column"_name},
           .position{0.0f, 0.0f, 0.0f},
           .rotation{1, 0, 0, 0},
           .scale{10, 10, 10},
   });

   m_scene.add_object(SceneObject{
           .model{"mdl:column"_name},
           .position{0.0f, 32.0f, 0.0f},
           .rotation{1, 0, 0, 0},
           .scale{10, 10, 10},
   });

   m_scene.add_object(SceneObject{
           .model{"mdl:column"_name},
           .position{0.0f, 16.0f, 0.0f},
           .rotation{1, 0, 0, 0},
           .scale{10, 10, 10},
   });

   m_scene.compile_scene();

   m_textRenderer.update_resolution(m_resolution);
   m_context2D.update_resolution(m_resolution);
}

void Renderer::on_render()
{
   const auto deltaTime = calculate_frame_duration();
   const auto framerate = calculate_framerate(deltaTime);

   const auto framerateStr = std::format("{}", framerate);
   m_textRenderer.update_text_object(m_glyphAtlas, m_framerateValue, framerateStr);
   const auto camPos      = m_scene.camera().position();
   const auto positionStr = std::format("{:.2f}, {:.2f}, {:.2f}", camPos.x, camPos.y, camPos.z);
   m_textRenderer.update_text_object(m_glyphAtlas, m_positionValue, positionStr);
   const auto orientationStr = std::format("{:.2f}, {:.2f}", m_pitch, m_yaw);
   m_textRenderer.update_text_object(m_glyphAtlas, m_orientationValue, orientationStr);
   const auto triangleCountStr = std::format("{}", m_commandList.triangle_count());
   m_textRenderer.update_text_object(m_glyphAtlas, m_triangleCountValue, triangleCountStr);
   m_textRenderer.update_text_object(m_glyphAtlas, m_aoValue, m_ssaoEnabled ? "Screen-Space" : "Off");
   m_textRenderer.update_text_object(m_glyphAtlas, m_aaValue, m_fxaaEnabled ? "FXAA" : "Off");

   const auto framebufferIndex = m_swapchain.get_available_framebuffer(m_framebufferReadySemaphore);
   this->update_uniform_data(deltaTime);
   m_inFlightFence.await();

   checkStatus(m_commandList.begin());
   m_context3D.set_active_command_list(&m_commandList);
   m_context2D.set_active_command_list(&m_commandList);

   {
      std::array<graphics_api::ClearValue, 1> clearValues{
              graphics_api::DepthStenctilValue{1.0f, 0.0f}
      };
      m_commandList.begin_render_pass(m_shadowMap.framebuffer(), clearValues);

      m_shadowMap.on_begin_render(m_context3D);
      m_scene.render_shadow_map();

      m_commandList.end_render_pass();
   }
   {
      std::array<graphics_api::ClearValue, 4> clearValues{
              graphics_api::ColorPalette::Black,
              graphics_api::ColorPalette::Black,
              graphics_api::ColorPalette::Black,
              graphics_api::DepthStenctilValue{1.0f, 0.0f},
      };
      m_commandList.begin_render_pass(m_modelFramebuffer, clearValues);

      m_skyBox.on_render(m_commandList, m_yaw, m_pitch, static_cast<float>(m_resolution.width),
                         static_cast<float>(m_resolution.height));

      m_groundRenderer.draw(m_commandList, m_scene.camera());

      m_context3D.begin_render();
      m_scene.render();

      if (m_showDebugLines) {
         m_debugLinesRenderer.begin_render(m_commandList);
         m_scene.render_debug_lines();
      }

      m_commandList.end_render_pass();
   }

   {
      std::array<graphics_api::ClearValue, 1> clearValues{
              graphics_api::ColorPalette::Black,
      };
      m_commandList.begin_render_pass(m_ambientOcclusionFramebuffer, clearValues);

      m_ambientOcclusionRenderer.draw(m_commandList, m_scene.camera().projection_matrix());

      m_commandList.end_render_pass();
   }

   {
      std::array<graphics_api::ClearValue, 1> clearValues{
              graphics_api::ColorPalette::Black,
      };
      m_commandList.begin_render_pass(m_shadingFramebuffer, clearValues);

      const auto shadowMat = m_scene.shadow_map_camera().view_projection_matrix() *
                             glm::inverse(m_scene.camera().view_matrix());
      const auto lightPosition =
              m_scene.camera().view_matrix() * glm::vec4(m_scene.shadow_map_camera().position(), 1.0);

      m_shadingRenderer.draw(m_commandList, glm::vec3(lightPosition), shadowMat, m_ssaoEnabled);

      m_commandList.end_render_pass();
   }

   {
      std::array<graphics_api::ClearValue, 3> clearValues{
              graphics_api::ColorPalette::Black,
              graphics_api::DepthStenctilValue{1.0f, 0.0f},
              graphics_api::ColorPalette::Black,
      };
      m_commandList.begin_render_pass(m_framebuffers[framebufferIndex], clearValues);

      m_postProcessingRenderer.draw(m_commandList, m_fxaaEnabled);

      m_rectangleRenderer.begin_render(m_commandList);
      m_rectangleRenderer.draw(m_commandList, m_renderPass, m_rectangle, m_resolution);

      // m_context2D.begin_render();
      // m_context2D.draw_sprite(m_sprite, {0.0f, 0.0f}, {0.2f, 0.2f});

      m_textRenderer.begin_render(m_commandList);
      auto textY = 16.0f + m_titleLabel.metric.height;
      m_textRenderer.draw_text_object(m_commandList, m_titleLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
      textY += 24.0f + m_framerateLabel.metric.height;
      m_textRenderer.draw_text_object(m_commandList, m_framerateLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
      m_textRenderer.draw_text_object(m_commandList, m_framerateValue,
                                      {16.0f + m_framerateLabel.metric.width + 8.0f, textY},
                                      {1.0f, 1.0f, 0.4f});
      textY += 12.0f + m_positionLabel.metric.height;
      m_textRenderer.draw_text_object(m_commandList, m_positionLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
      m_textRenderer.draw_text_object(m_commandList, m_positionValue,
                                      {16.0f + m_positionLabel.metric.width + 8.0f, textY},
                                      {1.0f, 1.0f, 0.4f});
      textY += 12.0f + m_orientationLabel.metric.height;
      m_textRenderer.draw_text_object(m_commandList, m_orientationLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
      m_textRenderer.draw_text_object(m_commandList, m_orientationValue,
                                      {16.0f + m_orientationLabel.metric.width + 8.0f, textY},
                                      {1.0f, 1.0f, 0.4f});
      textY += 12.0f + m_triangleCountLabel.metric.height;
      m_textRenderer.draw_text_object(m_commandList, m_triangleCountLabel, {16.0f, textY},
                                      {1.0f, 1.0f, 1.0f});
      m_textRenderer.draw_text_object(m_commandList, m_triangleCountValue,
                                      {16.0f + m_triangleCountLabel.metric.width + 8.0f, textY},
                                      {1.0f, 1.0f, 0.4f});
      textY += 12.0f + m_aoLabel.metric.height;
      m_textRenderer.draw_text_object(m_commandList, m_aoLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
      m_textRenderer.draw_text_object(m_commandList, m_aoValue,
                                      {16.0f + m_aoLabel.metric.width + 8.0f, textY}, {1.0f, 1.0f, 0.4f});
      textY += 12.0f + m_aaLabel.metric.height;
      m_textRenderer.draw_text_object(m_commandList, m_aaLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
      m_textRenderer.draw_text_object(m_commandList, m_aaValue,
                                      {16.0f + m_aaLabel.metric.width + 8.0f, textY}, {1.0f, 1.0f, 0.4f});

      m_commandList.end_render_pass();
   }

   checkStatus(m_commandList.finish());

   checkStatus(m_device->submit_command_list(m_commandList, m_framebufferReadySemaphore,
                                             m_renderFinishedSemaphore, m_inFlightFence));
   checkStatus(m_swapchain.present(m_renderFinishedSemaphore, framebufferIndex));
}

void Renderer::on_close() const
{
   m_inFlightFence.await();
}

void Renderer::on_mouse_relative_move(const float dx, const float dy)
{
   m_yaw -= dx * 0.01f;
   while (m_yaw < 0) {
      m_yaw += 2 * M_PI;
   }
   while (m_yaw >= 2 * M_PI) {
      m_yaw -= 2 * M_PI;
   }

   m_pitch += dy * 0.01f;
   m_pitch = std::clamp(m_pitch, -static_cast<float>(M_PI) / 2.0f + 0.01f,
                        static_cast<float>(M_PI) / 2.0f - 0.01f);

   m_scene.camera().set_orientation(glm::quat{
           glm::vec3{m_pitch, 0.0f, m_yaw}
   });
}

static Renderer::Moving map_direction(const uint32_t key)
{
   switch (key) {
   case 17: return Renderer::Moving::Foward;
   case 31: return Renderer::Moving::Backwards;
   case 30: return Renderer::Moving::Left;
   case 32: return Renderer::Moving::Right;
   case 18: return Renderer::Moving::Up;
   case 16: return Renderer::Moving::Down;
   }

   return Renderer::Moving::None;
}

void Renderer::on_key_pressed(const uint32_t key)
{
   if (const auto dir = map_direction(key); dir != Moving::None) {
      m_moveDirection = dir;
   }
   if (key == 61) {
      m_showDebugLines = not m_showDebugLines;
   }
   if (key == 62) {
      m_ssaoEnabled = not m_ssaoEnabled;
   }
   if (key == 63) {
      m_fxaaEnabled = not m_fxaaEnabled;
   }
   if (key == 57 && m_motion.z == 0.0f) {
      m_motion.z += -12.0f;
   }
}

void Renderer::on_key_released(const uint32_t key)
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

graphics_api::PipelineBuilder Renderer::create_pipeline()
{
   return {*m_device, m_modelRenderPass};
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

glm::vec3 Renderer::moving_direction() const
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

   m_textRenderer.update_resolution(resolution);
   m_context2D.update_resolution(resolution);

   m_device->await_all();

   m_modelColorTexture = checkResult(
           m_device->create_texture(g_colorFormat, resolution, graphics_api::TextureType::ColorAttachment));

   m_modelPositionTexture = checkResult(m_device->create_texture(GAPI_FORMAT(RGBA, Float16), resolution,
                                                                 graphics_api::TextureType::ColorAttachment));
   m_modelNormalTexture   = checkResult(m_device->create_texture(GAPI_FORMAT(RGBA, Float16), resolution,
                                                                 graphics_api::TextureType::ColorAttachment));
   m_modelDepthTexture    = checkResult(m_device->create_texture(g_depthFormat, resolution,
                                                                 graphics_api::TextureType::SampledDepthBuffer));

   m_ambientOcclusionTexture = checkResult(m_device->create_texture(
           GAPI_FORMAT(R, Float16), resolution, graphics_api::TextureType::ColorAttachment));

   m_shadingColorTexture = checkResult(m_device->create_texture(GAPI_FORMAT(RGBA, Float16), resolution,
                                                                graphics_api::TextureType::ColorAttachment));

   m_modelFramebuffer = checkResult(m_modelRenderTarget.create_framebuffer(
           m_modelRenderPass, m_modelColorTexture, m_modelPositionTexture, m_modelNormalTexture,
           m_modelDepthTexture));

   m_modelFramebuffer = checkResult(m_modelRenderTarget.create_framebuffer(
           m_modelRenderPass, m_modelColorTexture, m_modelPositionTexture, m_modelNormalTexture,
           m_modelDepthTexture));

   m_ambientOcclusionFramebuffer = checkResult(m_ambientOcclusionRenderTarget.create_framebuffer(
           m_ambientOcclusionRenderPass, m_ambientOcclusionTexture));

   m_shadingFramebuffer =
           checkResult(m_shadingRenderTarget.create_framebuffer(m_shadingRenderPass, m_shadingColorTexture));

   m_shadingRenderer.update_textures(m_modelColorTexture, m_modelPositionTexture, m_modelNormalTexture,
                                     m_ambientOcclusionTexture, m_shadowMap.depth_texture());

   m_ambientOcclusionRenderer.update_textures(m_modelPositionTexture, m_modelNormalTexture,
                                              m_resourceManager->texture("tex:noise"_name));

   m_postProcessingRenderer.update_texture(m_shadingColorTexture, m_modelDepthTexture);

   m_framebuffers.clear();

   m_swapchain = checkResult(
           m_device->create_swapchain(m_swapchain.color_format(), graphics_api::ColorSpace::sRGB,
                                      g_depthFormat, m_swapchain.sample_count(), resolution, &m_swapchain));
   m_renderPass   = checkResult(m_device->create_render_pass(m_swapchain));
   m_framebuffers = checkResult(m_swapchain.create_framebuffers(m_renderPass));

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

   m_scene.update();
}

}// namespace renderer
