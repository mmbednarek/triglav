#pragma once

#include "triglav/Math.hpp"
#include "triglav/String.hpp"
#include "triglav/Template.hpp"
#include "triglav/desktop/Desktop.hpp"

#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#define TG_UI_EVENTS                                          \
   TG_DEFINE_EVENT(MousePressed, on_mouse_pressed, Mouse)     \
   TG_DEFINE_EVENT(MouseReleased, on_mouse_released, Mouse)   \
   TG_DEFINE_EVENT_NO_PAYLOAD(MouseMoved, on_mouse_moved)     \
   TG_DEFINE_EVENT_NO_PAYLOAD(MouseEntered, on_mouse_entered) \
   TG_DEFINE_EVENT_NO_PAYLOAD(MouseLeft, on_mouse_left)       \
   TG_DEFINE_EVENT(MouseScrolled, on_mouse_scrolled, Scroll)  \
   TG_DEFINE_EVENT(KeyPressed, on_key_pressed, Keyboard)      \
   TG_DEFINE_EVENT(KeyReleased, on_key_released, Keyboard)    \
   TG_DEFINE_EVENT(TextInput, on_text_input, TextInput)       \
   TG_DEFINE_EVENT_NO_PAYLOAD(Undo, on_undo)                  \
   TG_DEFINE_EVENT_NO_PAYLOAD(Redo, on_redo)                  \
   TG_DEFINE_EVENT_NO_PAYLOAD(SelectAll, on_select_all)       \
   TG_DEFINE_EVENT_NO_PAYLOAD(Activated, on_activated)        \
   TG_DEFINE_EVENT_NO_PAYLOAD(Deactivated, on_deactivated)

namespace triglav::ui_core {

class Context;
class IWidget;

template<typename TWidget>
concept ConstructableWidget = requires(Context& ctx, typename TWidget::State&& state, IWidget* parent) {
   { TWidget{ctx, state, parent} } -> std::same_as<TWidget>;
};

struct Event
{
   enum class Type
   {
#define TG_DEFINE_EVENT(NAME, METHOD, PAYLOAD) NAME,
#define TG_DEFINE_EVENT_NO_PAYLOAD(NAME, METHOD) NAME,
      TG_UI_EVENTS
#undef TG_DEFINE_EVENT_NO_PAYLOAD
#undef TG_DEFINE_EVENT
   };

   struct Mouse
   {
      desktop::MouseButton button;
   };

   struct Scroll
   {
      float amount{};
   };

   struct Keyboard
   {
      desktop::Key key;
   };

   struct TextInput
   {
      Rune inputRune;
   };

   Type eventType;
   Vector2 parentSize;
   Vector2 mousePosition;
   Vector2 globalMousePosition;
   bool isForwardedToActive = false;
   std::variant<std::monostate, Mouse, Keyboard, TextInput, Scroll> data;

   Event sub_event(const Type event_type) const
   {
      Event result = *this;
      result.eventType = event_type;
      result.data = std::monostate{};
      return result;
   }
};

namespace event_visitor_concepts {

#define TG_DEFINE_EVENT(NAME, METHOD, PAYLOAD)                                                 \
   template<typename TVisitor, typename TResult>                                               \
   concept NAME = requires(TVisitor& visitor, const Event& e, const Event::PAYLOAD& payload) { \
      { visitor.METHOD(e, payload) } -> std::convertible_to<TResult>;                          \
   };
#define TG_DEFINE_EVENT_NO_PAYLOAD(NAME, METHOD)                \
   template<typename TVisitor, typename TResult>                \
   concept NAME = requires(TVisitor& visitor, const Event& e) { \
      { visitor.METHOD(e) } -> std::convertible_to<TResult>;    \
   };
TG_UI_EVENTS
#undef TG_DEFINE_EVENT_NO_PAYLOAD
#undef TG_DEFINE_EVENT

#define TG_DEFINE_EVENT(NAME, METHOD, PAYLOAD) NAME<TVisitor, TResult> ||

#define TG_DEFINE_EVENT_NO_PAYLOAD(NAME, METHOD) NAME<TVisitor, TResult> ||
template<typename TVisitor, typename TResult>
concept AnyVisitor = TG_UI_EVENTS false;
#undef TG_DEFINE_EVENT_NO_PAYLOAD
#undef TG_DEFINE_EVENT

}// namespace event_visitor_concepts

template<typename TResult, event_visitor_concepts::AnyVisitor<TResult> TEventVisitor>
TResult visit_event(TEventVisitor& visitor, const Event& ev, NonVoid<TResult> default_result = {})
{
   switch (ev.eventType) {
#define TG_DEFINE_EVENT(NAME, METHOD, PAYLOAD)                              \
   case Event::Type::NAME:                                                  \
      if constexpr (event_visitor_concepts::NAME<TEventVisitor, TResult>) { \
         return visitor.METHOD(ev, std::get<Event::PAYLOAD>(ev.data));      \
      }                                                                     \
      break;
#define TG_DEFINE_EVENT_NO_PAYLOAD(NAME, METHOD)                            \
   case Event::Type::NAME:                                                  \
      if constexpr (event_visitor_concepts::NAME<TEventVisitor, TResult>) { \
         return visitor.METHOD(ev);                                         \
      }                                                                     \
      break;
      TG_UI_EVENTS
#undef TG_DEFINE_EVENT_NO_PAYLOAD
#undef TG_DEFINE_EVENT
   }

   if constexpr (!std::is_same_v<TResult, void>) {
      return default_result;
   }
}

class IWidget
{
 public:
   virtual ~IWidget() = default;

   // Calculates widget's desired size based on parent's size.
   [[nodiscard]] virtual Vector2 desired_size(Vector2 parentSize) const = 0;

   // Adds widget to the current viewport
   // If the widget has already been added, it
   // will be adjusted to the new parent's dimensions.
   virtual void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) = 0;

   // Removed the widget from current viewport.
   virtual void remove_from_viewport() = 0;

   virtual void on_child_state_changed([[maybe_unused]] IWidget& widget) {};

   virtual void on_event([[maybe_unused]] const Event& event) {}
};

using IWidgetPtr = std::unique_ptr<IWidget>;

class BaseWidget : public IWidget
{
 public:
   explicit BaseWidget(IWidget* parent);

   void on_child_state_changed(IWidget& widget) override;

 protected:
   IWidget* m_parent;
};

class ContainerWidget : public BaseWidget
{
 public:
   ContainerWidget(Context& context, IWidget* parent);

   IWidget& set_content(IWidgetPtr&& content);

   template<typename T, typename... TArgs>
   T& emplace_content(TArgs&&... args)
   {
      return dynamic_cast<T&>(this->set_content(std::make_unique<T>(std::forward<TArgs>(args)...)));
   }

   template<ConstructableWidget T>
   T& create_content(typename T::State&& state)
   {
      return dynamic_cast<T&>(this->set_content(std::make_unique<T>(m_context, std::forward<typename T::State>(state), this)));
   }

 protected:
   Context& m_context;
   IWidgetPtr m_content;
};

class LayoutWidget : public BaseWidget
{
 public:
   LayoutWidget(Context& context, IWidget* parent);

   IWidget& add_child(IWidgetPtr&& widget);

   template<typename TChild, typename... TArgs>
   TChild& emplace_child(TArgs&&... args)
   {
      return dynamic_cast<TChild&>(this->add_child(std::make_unique<TChild>(std::forward<TArgs>(args)...)));
   }

   template<ConstructableWidget TChild>
   TChild& create_child(typename TChild::State&& state)
   {
      return dynamic_cast<TChild&>(this->add_child(std::make_unique<TChild>(m_context, std::forward<typename TChild::State>(state), this)));
   }

 protected:
   Context& m_context;
   std::vector<IWidgetPtr> m_children;
};

class ProxyWidget : public ContainerWidget
{
 public:
   ProxyWidget(Context& context, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const Event& event) override;
};

}// namespace triglav::ui_core