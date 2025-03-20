#pragma once

#include "../IWidget.hpp"

namespace triglav::ui_core {

class Context;

class EmptySpace final : public IWidget
{
 public:
   struct State
   {
      Vector2 size;
   };

   EmptySpace(Context& ctx, State state);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

 private:
   State m_state;
};

}// namespace triglav::ui_core