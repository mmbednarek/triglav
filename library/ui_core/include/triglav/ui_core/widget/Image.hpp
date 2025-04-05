#pragma once

#include "../IWidget.hpp"

#include "triglav/Name.hpp"

namespace triglav::ui_core {

class Context;

class Image final : public IWidget
{
 public:
   struct State
   {
      TextureName texture;
      std::optional<Vector2> maxSize;
      std::optional<Vector4> region;
   };

   Image(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;
   void set_region(Vector4 region);

 private:
   Context& m_context;
   State m_state;
   Name m_spriteID{};
};

}// namespace triglav::ui_core
