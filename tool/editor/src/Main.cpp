#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/launcher/Application.hpp"

#include "Editor.hpp"

int triglav_main(triglav::desktop::InputArgs& args, triglav::desktop::IDisplay& display)
{
   triglav::launcher::Application app(args, display);
   app.complete_stage();

   triglav::editor::Editor editor(app);
   return editor.run();
}
