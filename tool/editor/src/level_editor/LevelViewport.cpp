#include "LevelViewport.hpp"

#include "../RootWindow.hpp"

#include <numeric>

namespace triglav::editor {

namespace {

constexpr auto g_mouse_speed = 0.1f;

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
      return LevelViewport::CamMovement::Down;
   case desktop::Key::E:
      return LevelViewport::CamMovement::Up;
   default:
      return LevelViewport::CamMovement::None;
   }
}

}// namespace

LevelViewport::LevelViewport(IWidget* parent, RootWindow& root_window, LevelEditor& level_editor) :
    ui_core::BaseWidget(parent),
    m_root_window(root_window),
    m_level_editor(level_editor),
    TG_CONNECT(root_window.surface(), OnMouseRelativeMove, on_mouse_relative_move)
{
}

Vector2 LevelViewport::desired_size(const Vector2 parent_size) const
{
   return parent_size;
}

void LevelViewport::add_to_viewport(const Vector4 dimensions, Vector4 /*cropping_mask*/)
{
   m_dimensions = dimensions;
   m_level_editor.scene().update(graphics_api::Resolution{static_cast<u32>(dimensions.z), static_cast<u32>(dimensions.w)});
   m_render_viewport = std::make_unique<RenderViewport>(m_level_editor, dimensions);
   m_root_window.set_render_viewport(m_render_viewport.get());
}

void LevelViewport::remove_from_viewport()
{
   m_render_viewport.reset();
   m_dimensions = {0, 0, 0, 0};
}

void LevelViewport::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void LevelViewport::tick(const float delta_time)
{
   bool should_update_sm = false;
   const auto cam_orientation = m_level_editor.scene().camera().orientation();
   if (m_cam_movement != CamMovement::None) {
      Vector3 direction{};

      switch (m_cam_movement) {
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

      m_level_editor.scene().set_camera(
         m_level_editor.scene().camera().position() + m_level_editor.speed() * delta_time * (cam_orientation * direction), cam_orientation);
      should_update_sm = true;
   }

   const auto diff = delta_time * m_mouse_motion;
   m_level_editor.scene().update_orientation(diff.x, diff.y);
   m_mouse_motion += m_mouse_motion * (std::pow(0.5f, 50.0f * delta_time) - 1.0f);
   if (m_mouse_motion.x < 0.001f && m_mouse_motion.y < 0.001f) {
      m_mouse_motion = {};
   } else {
      should_update_sm = true;
   }

   if (should_update_sm) {
      m_level_editor.scene().update_shadow_maps();
      this->update_view();
   }
}

void LevelViewport::on_key_pressed(const ui_core::Event& /*event*/, const ui_core::Event::Keyboard& kb)
{
   if (!m_is_moving)
      return;

   const auto movement = to_cam_movement(kb.key);
   if (movement != CamMovement::None) {
      m_cam_movement = movement;
   }
}

void LevelViewport::on_key_released(const ui_core::Event& /*event*/, const ui_core::Event::Keyboard& kb)
{
   const auto movement = to_cam_movement(kb.key);
   if (movement == m_cam_movement) {
      m_cam_movement = CamMovement::None;
   }
}

void LevelViewport::update_view() const
{
   if (m_level_editor.selected_object() == nullptr)
      return;

   m_level_editor.tool().on_view_updated();
   m_level_editor.selection_tool().on_view_updated();
}

void LevelViewport::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& mouse)
{
   if (mouse.button == desktop::MouseButton::Right) {
      m_is_moving = true;
      m_root_window.surface().lock_cursor();
   } else if (mouse.button == desktop::MouseButton::Left) {
      const auto normalized_pos = event.mouse_position / rect_size(m_dimensions);
      const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);

      const auto ray = m_level_editor.scene().camera().viewport_ray(viewport_coord);
      if (!m_level_editor.tool().on_use_start(ray)) {
         m_level_editor.selection_tool().on_use_start(ray);
      }
   }
}

void LevelViewport::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   m_is_moving = false;
   m_cam_movement = CamMovement::None;
   m_mouse_motion = {};
   m_root_window.surface().unlock_cursor();
   m_level_editor.tool().on_use_end();
   m_level_editor.finish_using_tool();
}

void LevelViewport::on_mouse_moved(const ui_core::Event& event) const
{
   m_level_editor.tool().on_mouse_moved(event.mouse_position);
}

void LevelViewport::on_mouse_relative_move(const Vector2 difference)
{
   if (!m_is_moving)
      return;
   m_mouse_motion += g_mouse_speed * Vector2{-difference.x, difference.y};
}

Vector4 LevelViewport::dimensions() const
{
   return m_dimensions;
}

RenderViewport& LevelViewport::render_viewport() const
{
   return *m_render_viewport;
}

}// namespace triglav::editor
