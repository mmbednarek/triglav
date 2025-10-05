#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/Dialog.hpp"
#include "triglav/desktop_ui/DialogManager.hpp"
#include "triglav/desktop_ui/DropDownMenu.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/font/FontManager.hpp"
#include "triglav/io/CommandLine.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/resource/PathManager.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/threading/Scheduler.hpp"
#include "triglav/threading/ThreadPool.hpp"
#include "triglav/threading/Threading.hpp"
#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/ScrollBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <spdlog/spdlog.h>

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;
using triglav::desktop::WindowAttribute;
using triglav::desktop_ui::Dialog;
using triglav::desktop_ui::DialogManager;
using triglav::font::FontManger;
using triglav::io::CommandLine;
using triglav::render_core::GlyphCache;
using triglav::resource::PathManager;
using triglav::resource::ResourceManager;
using triglav::threading::ThreadPool;
using triglav::ui_core::HorizontalAlignment;
using triglav::ui_core::VerticalAlignment;

using namespace triglav::name_literals;
using namespace triglav::string_literals;
using namespace std::chrono_literals;

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

   const auto instance = GAPI_CHECK(triglav::graphics_api::Instance::create_instance(&display));

   std::unique_ptr<triglav::graphics_api::Device> device;

   {
      const auto surface = display.create_surface("Temporary Window"_strv, {32, 32}, WindowAttribute::Default);
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

   DialogManager dialogManager(instance, *device, display, glyphCache, resourceManager, {initialWidth, initialHeight});

   auto& alignmentBox = dialogManager.root().create_root_widget<triglav::ui_core::AlignmentBox>({
      .horizontalAlignment = HorizontalAlignment::Center,
      .verticalAlignment = VerticalAlignment::Center,
   });

   auto& globalLayout = alignmentBox.create_content<triglav::ui_core::VerticalLayout>({
      .padding = {0, 0, 0, 0},
      .separation = 0.0f,
   });

   auto& rect = globalLayout.create_child<triglav::ui_core::RectBox>({
      .color = {1, 0, 1, 1},
      .borderRadius = {0, 0, 0, 0},
      .borderColor = {0, 0, 0, 0},
      .borderWidth = 0,
   });
   rect.create_content<triglav::ui_core::EmptySpace>({.size = {200, 200}});

   auto& scroll = globalLayout.create_child<triglav::ui_core::ScrollBox>({.maxHeight = 300.0f});

   auto& rect2 = globalLayout.create_child<triglav::ui_core::RectBox>({
      .color = {1, 0, 1, 1},
      .borderRadius = {0, 0, 0, 0},
      .borderColor = {0, 0, 0, 0},
      .borderWidth = 0,
   });
   rect2.create_content<triglav::ui_core::EmptySpace>({.size = {200, 200}});

   auto& layout = scroll.create_content<triglav::ui_core::VerticalLayout>({
      .padding = {5.0f, 5.0f, 5.0f, 5.0f},
      .separation = 10.0f,
   });

   triglav::desktop_ui::DesktopUIManager desktopUiManager(
      triglav::desktop_ui::ThemeProperties{
         // Core
         .base_typeface = "cantarell.typeface"_rc,
         .background_color = {0.1f, 0.1f, 0.1f, 1.0f},
         .foreground_color = {1.0f, 1.0f, 1.0f, 1.0f},
         .accent_color = {0.0f, 0.0f, 1.0f, 1.0f},

         // Button
         .button_bg_color = {0.0f, 0.105f, 1.0f, 1.0f},
         .button_bg_hover_color = {0.035f, 0.22f, 1.0f, 1.0f},
         .button_bg_pressed_color = {0.0f, 0.015f, 0.48f, 1.0f},
         .button_font_size = 15,

         // Text
         .text_input_bg_inactive = {0.03f, 0.03f, 0.03f, 1.0f},
         .text_input_bg_active = {0.01f, 0.01f, 0.01f, 1.0f},
         .text_input_bg_hover = {0.035f, 0.035f, 0.035f, 1.0f},

         // Dropdown
         .dropdown_bg = {0.04f, 0.04f, 0.04f, 1.0f},
         .dropdown_bg_hover = {0.05f, 0.05f, 0.05f, 1.0f},
         .dropdown_border = {0.1f, 0.1f, 0.1f, 1.0f},
         .dropdown_border_width = 1.0f,
      },
      dialogManager.root().surface(), dialogManager);

   layout.create_child<triglav::desktop_ui::TextInput>({
      .manager = &desktopUiManager,
      .text = "Text Edit",
   });

   layout.create_child<triglav::desktop_ui::DropDownMenu>({
      .manager = &desktopUiManager,
      .items = {"Monday"_str, "Tuesday"_str, "Wednesday"_str, "Thursday"_str, "Friday"_str, "Saturday"_str, "Sunday"_str},
      .selectedItem = 0,
   });

   layout.create_child<triglav::desktop_ui::Button>({
      .manager = &desktopUiManager,
      .label = "Example",
   });

   layout.create_child<triglav::ui_core::Image>({
      .texture = "texture/ui_images.tex"_rc,
      .maxSize = triglav::Vector2{200, 200},
   });

   auto& rect3 = layout.create_child<triglav::ui_core::RectBox>({
      .color = {0, 1, 0, 1},
      .borderRadius = {0, 0, 0, 0},
      .borderColor = {0, 0, 0, 0},
      .borderWidth = 0,
   });
   rect3.create_content<triglav::ui_core::EmptySpace>({.size = {200, 300}});

   dialogManager.root().initialize();

   while (!dialogManager.should_close()) {
      dialogManager.tick();
      display.dispatch_messages();

      triglav::threading::Scheduler::the().tick();
   }

   device->await_all();

   ThreadPool::the().quit();

   return EXIT_SUCCESS;
}
