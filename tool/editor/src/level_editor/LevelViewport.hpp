#pragma once

#include "RenderViewport.hpp"

#include "triglav/desktop/ISurface.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::editor {

class RootWindow;

class LevelViewport final : public ui_core::BaseWidget
{
 public:
   enum class CamMovement
   {
      None,
      Forward,
      Backward,
      Left,
      Right,
      Up,
      Down,
   };

   using Self = LevelViewport;
   LevelViewport(IWidget* parent, RootWindow& root_window, LevelEditor& level_editor);

   [[nodiscard]] Vector2 desired_size(Vector2 available_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;
   void tick(float delta_time);

   void on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& kb);
   void on_key_released(const ui_core::Event& event, const ui_core::Event::Keyboard& kb);
   void update_view() const;
   void on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& mouse);
   void on_mouse_released(const ui_core::Event& event, const ui_core::Event::Mouse& mouse);
   void on_mouse_moved(const ui_core::Event& event) const;
   void on_mouse_relative_move(Vector2 difference);

   [[nodiscard]] Vector4 dimensions() const;
   [[nodiscard]] RenderViewport& render_viewport() const;
   [[nodiscard]] bool is_moving() const;

 private:
   Vector4 m_dimensions{};
   RootWindow& m_root_window;
   LevelEditor& m_level_editor;
   std::unique_ptr<RenderViewport> m_render_viewport;
   CamMovement m_cam_movement{CamMovement::None};
   bool m_is_moving{false};
   Vector2 m_mouse_motion{};

   TG_SINK(desktop::ISurface, OnMouseRelativeMove);
};

}// namespace triglav::editor