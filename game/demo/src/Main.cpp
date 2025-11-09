#include "GameInstance.hpp"

#include "triglav/Logging.hpp"
#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/io/CommandLine.hpp"
#include "triglav/resource/PathManager.hpp"
#include "triglav/threading/ThreadPool.hpp"

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;
using triglav::io::CommandLine;
using triglav::resource::PathManager;
using triglav::threading::ThreadPool;

using namespace triglav::name_literals;

constexpr auto g_defaultWidth = 1920;
constexpr auto g_defaultHeight = 1080;
constexpr auto g_minThreads = 1;
constexpr auto g_maxThreads = 64;

int triglav_main(InputArgs& args, IDisplay& display)
{
   using namespace triglav::string_literals;

   // Parse program arguments
   CommandLine::the().parse(args.arg_count, args.args);

   // Assign ID to the main thread
   triglav::threading::set_thread_id(triglav::threading::g_mainThread);

   // Initialize global thread pool
   const auto threadCount = CommandLine::the().arg_int("threadCount"_name).value_or(8);
   ThreadPool::the().initialize(std::clamp(threadCount, g_minThreads, g_maxThreads));

   const auto initialWidth = static_cast<triglav::u32>(CommandLine::the().arg_int("width"_name).value_or(g_defaultWidth));
   const auto initialHeight = static_cast<triglav::u32>(CommandLine::the().arg_int("height"_name).value_or(g_defaultHeight));

   std::string_view content_path{PathManager::the().content_path().string()};
   triglav::log_message(triglav::LogLevel::Info, "DemoGame"_strv, "content path: {}", content_path);
   std::string_view build_path{PathManager::the().build_path().string()};
   triglav::log_message(triglav::LogLevel::Info, "DemoGame"_strv, "build path: {}", build_path);
   triglav::log_message(triglav::LogLevel::Info, "DemoGame"_strv, "initializing renderer");

   demo::GameInstance instance(display, {initialWidth, initialHeight});
   instance.loop(display);

   ThreadPool::the().quit();

   return EXIT_SUCCESS;
}
