#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/project/Name.hpp"

#include "Editor.hpp"

TG_PROJECT_NAME(triglav_editor)

int triglav_main(triglav::desktop::InputArgs& args, triglav::desktop::IDisplay& display)
{
   triglav::editor::Editor editor(args, display);
   return editor.run();
}
