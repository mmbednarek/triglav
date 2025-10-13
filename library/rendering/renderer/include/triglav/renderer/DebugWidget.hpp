#pragma once

#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"

namespace triglav::renderer {

class DebugWidget final : public ui_core::IWidget
{
 public:
   explicit DebugWidget(ui_core::Context& ctx);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;

 private:
   ui_core::RectBox m_content;
};

}// namespace triglav::renderer