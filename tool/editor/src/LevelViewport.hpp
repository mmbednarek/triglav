#pragma once

#include "RenderViewport.hpp"

#include "triglav/ui_core/IWidget.hpp"

namespace triglav::editor {

class RootWindow;

class LevelViewport final : public ui_core::BaseWidget
{
 public:
   using Self = LevelViewport;
   LevelViewport(IWidget* parent, RootWindow& rootWindow, LevelEditor& levelEditor);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;

   [[nodiscard]] Vector4 dimensions() const;

 private:
   Vector4 m_dimensions{};
   RootWindow& m_rootWindow;
   LevelEditor& m_levelEditor;
   std::unique_ptr<RenderViewport> m_renderViewport;
};

}// namespace triglav::editor