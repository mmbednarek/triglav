#pragma once

#include "triglav/Math.hpp"
#include "triglav/String.hpp"
#include "triglav/desktop/Desktop.hpp"

#include <memory>
#include <variant>
#include <vector>

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
      MousePressed,
      MouseReleased,
      MouseMoved,
      MouseEntered,
      MouseLeft,
      MouseScrolled,
      KeyPressed,
      TextInput,
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
   std::variant<std::monostate, Mouse, Keyboard, TextInput, Scroll> data;
};

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

}// namespace triglav::ui_core