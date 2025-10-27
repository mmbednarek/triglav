#include "LevelViewport.hpp"

#include "../../../../../../.conan2/p/b/boost685345d9ed20d/p/include/boost/algorithm/minmax_element.hpp"
#include "RootWindow.hpp"

#include <spdlog/spdlog.h>

namespace triglav::editor {

namespace {

constexpr auto g_mouseSpeed = 0.1f;

LevelViewport::CamMovement to_cam_movement(const desktop::Key key)
{
   switch (key) {
   case desktop::Key::W:
      return LevelViewport::CamMovement::Forward;
   case desktop::Key::S:
      return LevelViewport::CamMovement::Backward;
   case desktop::Key::A:
      return LevelViewport::CamMovement::Left;
   case desktop::Key::D:
      return LevelViewport::CamMovement::Right;
   case desktop::Key::Q:
      return LevelViewport::CamMovement::Up;
   case desktop::Key::E:
      return LevelViewport::CamMovement::Down;
   default:
      return LevelViewport::CamMovement::None;
   }
}

}// namespace

LevelViewport::LevelViewport(IWidget* parent, RootWindow& rootWindow, LevelEditor& levelEditor) :
    ui_core::BaseWidget(parent),
    m_rootWindow(rootWindow),
    m_levelEditor(levelEditor),
    TG_CONNECT(rootWindow.surface(), OnMouseRelativeMove, on_mouse_relative_move)
{
}

Vector2 LevelViewport::desired_size(const Vector2 parentSize) const
{
   return parentSize;
}

void LevelViewport::add_to_viewport(const Vector4 dimensions, Vector4 /*croppingMask*/)
{
   m_dimensions = dimensions;
   m_levelEditor.scene().update(graphics_api::Resolution{static_cast<u32>(dimensions.z), static_cast<u32>(dimensions.w)});
   m_renderViewport = std::make_unique<RenderViewport>(m_levelEditor, dimensions);
   m_rootWindow.set_render_viewport(m_renderViewport.get());
}

void LevelViewport::remove_from_viewport()
{
   m_renderViewport.reset();
   m_dimensions = {0, 0, 0, 0};
}

void LevelViewport::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void LevelViewport::tick(const float delta_time)
{
   const auto cam_orientation = m_levelEditor.scene().camera().orientation();
   if (m_camMovement != CamMovement::None) {
      Vector3 direction{};

      switch (m_camMovement) {
      case CamMovement::Forward: {
         direction = Vector3{0, 1, 0};
         break;
      }
      case CamMovement::Backward: {
         direction = Vector3{0, -1, 0};
         break;
      }
      case CamMovement::Left: {
         direction = Vector3{-1, 0, 0};
         break;
      }
      case CamMovement::Right: {
         direction = Vector3{1, 0, 0};
         break;
      }
      case CamMovement::Up: {
         direction = Vector3{0, 0, -1};
         break;
      }
      case CamMovement::Down: {
         direction = Vector3{0, 0, 1};
         break;
      }
      default:
         break;
      }

      m_levelEditor.scene().set_camera(m_levelEditor.scene().camera().position() + 10.0f * delta_time * (cam_orientation * direction),
                                       cam_orientation);
   }

   const auto diff = delta_time * m_mouseMotion;
   m_levelEditor.scene().update_orientation(diff.x, diff.y);
   m_mouseMotion += m_mouseMotion * (static_cast<float>(pow(0.5f, 50.0f * delta_time)) - 1.0f);
   if (m_mouseMotion.x < 0.001f && m_mouseMotion.y < 0.001f) {
      m_mouseMotion = {};
   }
}

void LevelViewport::on_key_pressed(const ui_core::Event& /*event*/, const ui_core::Event::Keyboard& kb)
{
   if (!m_isMoving)
      return;

   const auto movement = to_cam_movement(kb.key);
   if (movement != CamMovement::None) {
      m_camMovement = movement;
   }
}

void LevelViewport::on_key_released(const ui_core::Event& /*event*/, const ui_core::Event::Keyboard& kb)
{
   const auto movement = to_cam_movement(kb.key);
   if (movement == m_camMovement) {
      m_camMovement = CamMovement::None;
   }
}

void LevelViewport::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& mouse)
{
   if (mouse.button == desktop::MouseButton::Right) {
      m_isMoving = true;
      m_rootWindow.surface().lock_cursor();
   } else if (mouse.button == desktop::MouseButton::Left) {
      const auto normalized_pos = event.mousePosition / rect_size(m_dimensions);
      const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);

      const auto ray = m_levelEditor.scene().camera().viewport_ray(viewport_coord);
      const auto result = m_levelEditor.scene().trace_ray(ray);
      if (result != nullptr) {
         spdlog::info("BVH Hit! {}", m_rootWindow.resource_manager().lookup_name(result->model).value_or("unknown"));
      } else {
         spdlog::info("No Hit");
      }
   }
}

void LevelViewport::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   m_isMoving = false;
   m_camMovement = CamMovement::None;
   m_mouseMotion = {};
   m_rootWindow.surface().unlock_cursor();
}

void LevelViewport::on_mouse_relative_move(const Vector2 difference)
{
   if (!m_isMoving)
      return;
   m_mouseMotion += g_mouseSpeed * Vector2{-difference.x, difference.y};
}

Vector4 LevelViewport::dimensions() const
{
   return m_dimensions;
}

}// namespace triglav::editor
