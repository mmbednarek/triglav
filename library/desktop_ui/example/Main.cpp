#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/Dialog.hpp"
#include "triglav/font/FontManager.hpp"
#include "triglav/io/CommandLine.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/resource/PathManager.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/threading/ThreadPool.hpp"
#include "triglav/threading/Threading.hpp"
#include "triglav/ui_core/widget/AlignmentBox.hpp"

#include <spdlog/spdlog.h>

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;
using triglav::desktop::WindowAttribute;
using triglav::desktop_ui::Dialog;
using triglav::font::FontManger;
using triglav::io::CommandLine;
using triglav::render_core::GlyphCache;
using triglav::resource::PathManager;
using triglav::resource::ResourceManager;
using triglav::threading::ThreadPool;
using triglav::ui_core::HorizontalAlignment;
using triglav::ui_core::VerticalAlignment;

using namespace triglav::name_literals;

constexpr auto g_defaultWidth = 800;
constexpr auto g_defaultHeight = 600;

TG_DEFINE_AWAITER(ResourceLoadedAwaiter, ResourceManager, OnLoadedAssets)

int triglav_main(InputArgs& args, IDisplay& display)
{
   // Parse program arguments
   CommandLine::the().parse(args.arg_count, args.args);

   // Assign ID to the main thread
   triglav::threading::set_thread_id(triglav::threading::g_mainThread);

   // Initialize global thread pool
   const auto threadCount = CommandLine::the().arg_int("threadCount"_name).value_or(8);
   ThreadPool::the().initialize(std::clamp(threadCount, 0, 8));

   const auto instance = GAPI_CHECK(triglav::graphics_api::Instance::create_instance());

   std::unique_ptr<triglav::graphics_api::Device> device;

   {
      const auto surface = display.create_surface("Temporary Window", {32, 32}, WindowAttribute::Default);
      const auto surfaceGfx = GAPI_CHECK(instance.create_surface(*surface));

      device = GAPI_CHECK(instance.create_device(&surfaceGfx, triglav::graphics_api::DevicePickStrategy::PreferDedicated,
                                                 triglav::graphics_api::DeviceFeature::None));
   }

   FontManger fontManager;
   ResourceManager resourceManager(*device, fontManager);

   GlyphCache glyphCache(*device, resourceManager);

   ResourceLoadedAwaiter resourceAwaiter(resourceManager);
   resourceManager.load_asset_list(PathManager::the().content_path().sub("index_base.yaml"));
   resourceAwaiter.await();

   const auto initialWidth = static_cast<triglav::u32>(CommandLine::the().arg_int("width"_name).value_or(g_defaultWidth));
   const auto initialHeight = static_cast<triglav::u32>(CommandLine::the().arg_int("height"_name).value_or(g_defaultHeight));

   Dialog dialog(instance, *device, display, glyphCache, resourceManager, {initialWidth, initialHeight});

   auto& alignmentBox = dialog.create_root_widget<triglav::ui_core::AlignmentBox>({
      .horizontalAlignment = HorizontalAlignment::Center,
      .verticalAlignment = VerticalAlignment::Center,
   });

   const triglav::desktop_ui::DesktopUIManager desktopUiManager(triglav::desktop_ui::ThemeProperties{
      .background_color = {0.1f, 0.1f, 0.1f, 1.0f},
      .foreground_color = {0.9f, 0.9f, 0.9f, 1.0f},
      .accent_color = {0.0f, 0.0f, 1.0f, 1.0f},
      .button_bg_color = {0.1f, 0.1f, 0.15f, 1.0f},
      .button_bg_hover_color = {0.2f, 0.2f, 0.25f, 1.0f},
      .button_bg_pressed_color = {0.05f, 0.05f, 0.15f, 1.0f},
      .button_font_size = 15,
      .base_typeface = "cantarell.typeface"_rc,
   });

   alignmentBox.create_content<triglav::desktop_ui::Button>({
      .manager = &desktopUiManager,
      .label = "Example",
   });

   dialog.initialize();

   while (!dialog.should_close()) {
      dialog.update();
      display.dispatch_messages();
   }

   ThreadPool::the().quit();

   return EXIT_SUCCESS;
}
