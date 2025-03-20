#pragma once

#include "../IWidget.hpp"

#include "triglav/Name.hpp"

namespace triglav::ui_core {

class Context;

class RectBox final : public IWidget
{
 public:
   struct State
   {
      Vector4 color;
   };

   RectBox(Context& context, State state);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   IWidget& set_content(IWidgetPtr&& content);

   template<typename T, typename... TArgs>
   T& emplace_content(TArgs&&... args)
   {
      return dynamic_cast<T&>(this->set_content(std::make_unique<T>(std::forward<TArgs>(args)...)));
   }

   template<typename T>
   T& create_content(typename T::State&& state)
   {
      return dynamic_cast<T&>(this->set_content(std::make_unique<T>(m_context, std::forward<typename T::State>(state))));
   }

 private:
   Context& m_context;

   State m_state{};
   IWidgetPtr m_content;
   Name m_rectName{};
   mutable Vector2 m_cachedParentSize{};
   mutable Vector2 m_cachedSize{};
};

}// namespace triglav::ui_core