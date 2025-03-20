#pragma once

#include "triglav/Name.hpp"
#include "triglav/ui_core/IWidget.hpp"

#include <optional>
#include <string>

namespace triglav::ui_core {

class Context;

class TextBox final : public IWidget
{
 public:
   struct State
   {
      i32 fontSize{};
      TypefaceName typeface{};
      std::string content{};
      Vector4 color{};
   };

   TextBox(Context& ctx, State initialState);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   void set_content(std::string_view content);

 private:
   Context& m_uiContext;

   Name m_id{};
   State m_state{};
   mutable std::optional<Vector2> m_cachedDesiredSize{};
};

}// namespace triglav::ui_core