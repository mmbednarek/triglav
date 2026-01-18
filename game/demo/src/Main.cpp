#include "GameInstance.hpp"

#include "../../../library/engine/project/include/triglav/project/PathManager.hpp"
#include "triglav/BuildInfo.hpp"
#include "triglav/Logging.hpp"
#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/io/CommandLine.hpp"
#include "triglav/io/Logging.hpp"
#include "triglav/project/Name.hpp"
#include "triglav/project/ProjectManager.hpp"
#include "triglav/threading/ThreadPool.hpp"

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;
using triglav::io::CommandLine;
using triglav::project::PathManager;
using triglav::threading::ThreadPool;

using namespace triglav::name_literals;

constexpr auto g_default_width = 1920;
constexpr auto g_default_height = 1080;
constexpr auto g_min_threads = 1;
constexpr auto g_max_threads = 64;

TG_PROJECT_NAME(demo)

int triglav_main(InputArgs& args, IDisplay& display)
{
   using namespace triglav::string_literals;

   [[maybe_unused]] auto proj = triglav::project::ProjectManager::the().project_root("demo"_name);

   // Parse program arguments
   CommandLine::the().parse(args.arg_count, args.args);

   // Initialize logger
   triglav::LogManager::the().register_listener<triglav::io::StreamLogger>(triglav::io::stdout_writer());

   // Assign ID to the main thread
   triglav::threading::set_thread_id(triglav::threading::g_main_thread);

   // Initialize global thread pool
   const auto thread_count = CommandLine::the().arg_int("threadCount"_name).value_or(8);
   ThreadPool::the().initialize(std::clamp(thread_count, g_min_threads, g_max_threads));

   const auto initial_width = static_cast<triglav::u32>(CommandLine::the().arg_int("width"_name).value_or(g_default_width));
   const auto initial_height = static_cast<triglav::u32>(CommandLine::the().arg_int("height"_name).value_or(g_default_height));

   triglav::log_message(triglav::LogLevel::Info, "DemoGame"_strv, "build profile: " TG_STRING(TG_BUILD_PROFILE));
   const auto content_path = PathManager::the().translate_path("."_rc);
   triglav::log_message(triglav::LogLevel::Info, "DemoGame"_strv, "content path: {}", content_path.string());
   triglav::log_message(triglav::LogLevel::Info, "DemoGame"_strv, "initializing renderer");

   demo::GameInstance instance(display, {initial_width, initial_height});
   instance.loop(display);

   ThreadPool::the().quit();

   return EXIT_SUCCESS;
}
