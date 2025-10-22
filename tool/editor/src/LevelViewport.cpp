#include "LevelViewport.hpp"

#include "RootWindow.hpp"

namespace triglav::editor {

LevelViewport::LevelViewport(IWidget* parent, RootWindow& rootWindow, LevelEditor& levelEditor) :
    ui_core::BaseWidget(parent),
    m_rootWindow(rootWindow),
    m_levelEditor(levelEditor)
{
}

Vector2 LevelViewport::desired_size(const Vector2 parentSize) const
{
   return parentSize;
}

void LevelViewport::add_to_viewport(const Vector4 dimensions, Vector4 /*croppingMask*/)
{
   m_dimensions = dimensions;
   m_renderViewport = std::make_unique<RenderViewport>(m_levelEditor, dimensions);
   m_rootWindow.set_render_viewport(m_renderViewport.get());
}

void LevelViewport::remove_from_viewport()
{
   m_renderViewport.reset();
   m_dimensions = {0, 0, 0, 0};
}

Vector4 LevelViewport::dimensions() const
{
   return m_dimensions;
}

}// namespace triglav::editor
