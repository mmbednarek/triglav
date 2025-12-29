#pragma once

#include "../IWidget.hpp"
#include "../Primitives.hpp"

#include "triglav/Name.hpp"

#include <optional>

namespace triglav::ui_core {

class Context;

class Image final : public IWidget
{
 public:
   struct State
   {
      TextureName texture;
      std::optional<Vector2> max_size;
      std::optional<Vector4> region;
   };

   Image(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 available_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void set_region(Vector4 region);

 private:
   Context& m_context;
   State m_state;
   SpriteId m_sprite_id{};
};

}// namespace triglav::ui_core
