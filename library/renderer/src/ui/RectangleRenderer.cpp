#include "ui/RectangleRenderer.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/render_core/BuildContext.hpp"

namespace triglav::renderer::ui {

using namespace name_literals;

RectangleRenderer::RectangleRenderer(graphics_api::Device& device, ui_core::Viewport& viewport) :
    m_device(device),
    m_viewport(viewport),
    TG_CONNECT(viewport, OnAddedRectangle, on_added_rectangle),
    TG_CONNECT(viewport, OnRectangleChangeDims, on_rectangle_change_dims),
    TG_CONNECT(viewport, OnRectangleChangeColor, on_rectangle_change_color)
{
}

void RectangleRenderer::on_added_rectangle(Name name, const ui_core::Rectangle& rect)
{
   std::unique_lock lk{m_rectUpdateMtx};

   const std::array vertices{
      Vector2{rect.rect.x, rect.rect.y}, Vector2{rect.rect.x, rect.rect.w}, Vector2{rect.rect.z, rect.rect.y},
      Vector2{rect.rect.z, rect.rect.y}, Vector2{rect.rect.x, rect.rect.w}, Vector2{rect.rect.z, rect.rect.w},
   };

   auto vertexBuffer = GAPI_CHECK(
      m_device.create_buffer(graphics_api::BufferUsage::VertexBuffer | graphics_api::BufferUsage::TransferDst, sizeof(vertices)));
   GAPI_CHECK_STATUS(vertexBuffer.write_indirect(vertices.data(), sizeof(vertices)));

   struct VertUBO
   {
      Vector2 position;
      Vector2 viewportSize;
   } vsUbo{};
   vsUbo.position = {rect.rect.x, rect.rect.y};
   vsUbo.viewportSize = m_viewport.dimensions();

   auto vsUboBuffer = GAPI_CHECK(
      m_device.create_buffer(graphics_api::BufferUsage::UniformBuffer | graphics_api::BufferUsage::TransferDst, sizeof(VertUBO)));
   GAPI_CHECK_STATUS(vsUboBuffer.write_indirect(&vsUbo, sizeof(VertUBO)));

   struct FragUBO
   {
      Vector4 borderRadius;
      Vector4 backgroundColor;
      Vector2 rectSize;
   } fsUbo{};
   fsUbo.borderRadius = {10, 10, 10, 10};
   fsUbo.backgroundColor = rect.color;
   fsUbo.rectSize = {rect.rect.z - rect.rect.x, rect.rect.w - rect.rect.y};

   auto fsUboBuffer = GAPI_CHECK(
      m_device.create_buffer(graphics_api::BufferUsage::UniformBuffer | graphics_api::BufferUsage::TransferDst, sizeof(FragUBO)));
   GAPI_CHECK_STATUS(fsUboBuffer.write_indirect(&fsUbo, sizeof(FragUBO)));

   m_rectangles.emplace(name, RectangleData{std::move(vsUboBuffer), std::move(fsUboBuffer), std::move(vertexBuffer)});
}

void RectangleRenderer::on_rectangle_change_dims(const Name name, const ui_core::Rectangle& rect)
{
   std::unique_lock lk{m_rectUpdateMtx};

   auto& dstRect = m_rectangles.at(name);

   const std::array vertices{
      Vector2{rect.rect.x, rect.rect.y}, Vector2{rect.rect.x, rect.rect.w}, Vector2{rect.rect.z, rect.rect.y},
      Vector2{rect.rect.z, rect.rect.y}, Vector2{rect.rect.x, rect.rect.w}, Vector2{rect.rect.z, rect.rect.w},
   };

   GAPI_CHECK_STATUS(dstRect.vertexBuffer.write_indirect(vertices.data(), sizeof(vertices)));

   struct VertUBO
   {
      Vector2 position;
      Vector2 viewportSize;
   } vsUbo{};
   vsUbo.position = {rect.rect.x, rect.rect.y};
   vsUbo.viewportSize = m_viewport.dimensions();

   GAPI_CHECK_STATUS(dstRect.vsUbo.write_indirect(&vsUbo, sizeof(VertUBO)));

   struct FragUBO
   {
      Vector4 borderRadius;
      Vector4 backgroundColor;
      Vector2 rectSize;
   } fsUbo{};
   fsUbo.borderRadius = {10, 10, 10, 10};
   fsUbo.backgroundColor = rect.color;
   fsUbo.rectSize = {rect.rect.z - rect.rect.x, rect.rect.w - rect.rect.y};

   GAPI_CHECK_STATUS(dstRect.fsUbo.write_indirect(&fsUbo, sizeof(FragUBO)));
}

void RectangleRenderer::on_rectangle_change_color(const Name name, const ui_core::Rectangle& rect)
{
   std::unique_lock lk{m_rectUpdateMtx};

   auto& dstRect = m_rectangles.at(name);

   struct FragUBO
   {
      Vector4 borderRadius;
      Vector4 backgroundColor;
      Vector2 rectSize;
   } fsUbo{};
   fsUbo.borderRadius = {10, 10, 10, 10};
   fsUbo.backgroundColor = rect.color;
   fsUbo.rectSize = {rect.rect.z - rect.rect.x, rect.rect.w - rect.rect.y};

   GAPI_CHECK_STATUS(dstRect.fsUbo.write_indirect(&fsUbo, sizeof(FragUBO)));
}

static const auto g_rectangleLayout = render_core::VertexLayout(sizeof(Vector2)).add("position"_name, GAPI_FORMAT(RG, Float32), 0);

void RectangleRenderer::build_render_ui(render_core::BuildContext& ctx)
{
   for (const auto& rectangle : Values(m_rectangles)) {
      ctx.bind_vertex_shader("rectangle.vshader"_rc);

      ctx.bind_uniform_buffer(0, &rectangle.vsUbo);

      ctx.bind_fragment_shader("rectangle.fshader"_rc);

      ctx.bind_uniform_buffer(1, &rectangle.fsUbo);

      ctx.bind_vertex_layout(g_rectangleLayout);
      ctx.bind_vertex_buffer(&rectangle.vertexBuffer);

      ctx.draw_primitives(6, 0);
   }
}

}// namespace triglav::renderer::ui
