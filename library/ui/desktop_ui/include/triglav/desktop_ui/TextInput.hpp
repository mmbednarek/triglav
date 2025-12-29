#pragma once

#include "DesktopUI.hpp"

#include "triglav/Logging.hpp"
#include "triglav/String.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/PrimitiveHelpers.hpp"
#include "triglav/ui_core/Primitives.hpp"

namespace triglav::ui_core {
class TextBox;
class RectBox;
}// namespace triglav::ui_core
namespace triglav::desktop_ui {

class TextInput final : public ui_core::IWidget
{
   TG_DEFINE_LOG_CATEGORY(TextInput)
 public:
   using Self = TextInput;

   TG_EVENT(OnTextChanged, StringView)
   TG_EVENT(OnTyping, StringView)

   struct State
   {
      DesktopUIManager* manager;
      String text;
      std::function<bool(Rune)> filter_func = [](Rune) { return true; };
      Color border_color = {0.1f, 0.1f, 0.1f, 1.0f};
   };

   TextInput(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 available_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;
   void update_carret_state();
   void set_content(StringView content);
   [[nodiscard]] const String& content() const;

   void on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& mouse);
   void on_mouse_released(const ui_core::Event& event, const ui_core::Event::Mouse& mouse);
   void on_mouse_moved(const ui_core::Event& event);
   void on_mouse_entered(const ui_core::Event& event);
   void on_mouse_left(const ui_core::Event& event);
   void on_text_input(const ui_core::Event& event, const ui_core::Event::TextInput& text_input);
   void on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard);
   void on_activated(const ui_core::Event& event);
   void on_deactivated(const ui_core::Event& event);
   void on_select_all(const ui_core::Event& event);

 private:
   void recalculate_caret_offset(bool removal = false);
   void update_text_position();
   void update_selection_box();
   Vector4 text_cropping_mask() const;
   u32 rune_index_from_offset(float offset) const;
   void disable_caret();
   void enable_caret();
   void remove_selected();

   ui_core::Context& m_context;
   State m_state;

   ui_core::RectInstance m_background_rect;
   ui_core::RectInstance m_selection_rect;
   ui_core::TextInstance m_text_prim;
   ui_core::RectInstance m_caret_box;
   bool m_is_carret_visible{false};
   bool m_is_selecting{false};
   u32 m_caret_position{};
   u32 m_selected_count{};
   Vector4 m_dimensions{};
   Vector4 m_cropping_mask{};
   Vector2 m_text_size{};
   float m_text_xposition{};
   float m_text_offset{};
   float m_caret_offset{};
   float m_text_width{};

   bool m_is_active{false};
   std::optional<u32> m_timeout_handle{};
};


}// namespace triglav::desktop_ui
