#pragma once

#include "triglav/desktop_ui/DesktopUI.hpp"

namespace triglav::editor {

class LevelEditor;

class UIContext : public desktop_ui::DesktopContext
{
 public:
   UIContext(ui_core::Viewport& viewport, render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager,
             desktop_ui::ThemeProperties properties, desktop::ISurface& surface, desktop_ui::PopupManager& dialog_manager);

 private:
};

}// namespace triglav::editor
