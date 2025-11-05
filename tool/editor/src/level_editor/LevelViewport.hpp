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
   LevelViewport(IWidget* parent, RootWindow& rootWindow, LevelEditor& levelEditor);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;
   void tick(float delta_time);

   void on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& kb);
   void on_key_released(const ui_core::Event& event, const ui_core::Event::Keyboard& kb);
   void update_viewport_helpers(const renderer::SceneObject* object);
   void on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& mouse);
   void on_mouse_released(const ui_core::Event& event, const ui_core::Event::Mouse& mouse);
   void on_mouse_moved(const ui_core::Event& event);
   void on_mouse_relative_move(Vector2 difference);

   [[nodiscard]] Vector4 dimensions() const;

 private:
   Vector4 m_dimensions{};
   RootWindow& m_rootWindow;
   LevelEditor& m_levelEditor;
   std::unique_ptr<RenderViewport> m_renderViewport;
   CamMovement m_camMovement{CamMovement::None};
   bool m_isMoving{false};
   Vector2 m_mouseMotion{};
   const renderer::SceneObject* m_selectedObject{};
   renderer::ObjectID m_selectedObjectID{};
   std::optional<Axis> m_transformAxis;
   Vector3 m_translationOffset{};

   geometry::BoundingBox m_arrow_x_bb{};
   geometry::BoundingBox m_arrow_y_bb{};
   geometry::BoundingBox m_arrow_z_bb{};

   TG_SINK(desktop::ISurface, OnMouseRelativeMove);
};

}// namespace triglav::editor