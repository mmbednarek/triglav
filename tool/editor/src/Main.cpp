#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/launcher/Application.hpp"

#include "Editor.hpp"

int triglav_main(triglav::desktop::InputArgs& args, triglav::desktop::IDisplay& display)
{
   triglav::editor::Editor editor(args, display);
   return editor.run();
}

