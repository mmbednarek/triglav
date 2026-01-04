#pragma once

#include "triglav/event/Delegate.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/PrimitiveHelpers.hpp"
#include "triglav/ui_core/UICore.hpp"

namespace triglav::desktop_ui {

class DesktopUIManager;

enum class SplitterOffsetType
{
   Preceeding,
   Following,
};

class Splitter final : public ui_core::BaseWidget
{
 public:
   using Self = Splitter;

   struct State
   {
      DesktopUIManager* manager;
      float offset;
      ui_core::Axis axis;
      SplitterOffsetType offset_type;
   };

   Splitter(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 available_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   IWidget& set_preceding(ui_core::IWidgetPtr&& widget);
   IWidget& set_following(ui_core::IWidgetPtr&& widget);

   template<ui_core::ConstructableWidget T>
   T& create_preceding(typename T::State&& state)
   {
      return dynamic_cast<T&>(this->set_preceding(std::make_unique<T>(m_context, std::forward<typename T::State>(state), this)));
   }

   template<ui_core::ConstructableWidget T>
   T& create_following(typename T::State&& state)
   {
      return dynamic_cast<T&>(this->set_following(std::make_unique<T>(m_context, std::forward<typename T::State>(state), this)));
   }

 private:
   float offset() const;
   void add_offset(float diff);
   TG_DECLARE_AXIS_FUNCS(m_state.axis)

   ui_core::Context& m_context;
   State m_state;
   ui_core::IWidgetPtr m_preceding;
   ui_core::IWidgetPtr m_following;
   bool m_is_moving{false};
   bool m_is_showing_cursor{false};
   float m_last_mouse_pos{};
   Vector4 m_dimensions{};
   Vector4 m_cropping_mask{};
   ui_core::RectInstance m_background{};
};

}// namespace triglav::desktop_ui
