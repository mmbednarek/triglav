#include "UIContext.hpp"

namespace triglav::editor {

UIContext::UIContext(ui_core::Viewport& viewport, render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager,
                     desktop_ui::ThemeProperties properties, desktop::ISurface& surface, desktop_ui::PopupManager& dialog_manager) :
    desktop_ui::DesktopContext(viewport, glyph_cache, resource_manager, std::move(properties), surface, dialog_manager)
{
}

}// namespace triglav::editor
