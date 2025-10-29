#include "triglav/Format.hpp"
#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/CheckBox.hpp"
#include "triglav/desktop_ui/ContextMenu.hpp"
#include "triglav/desktop_ui/Dialog.hpp"
#include "triglav/desktop_ui/DropDownMenu.hpp"
#include "triglav/desktop_ui/MenuBar.hpp"
#include "triglav/desktop_ui/PopupManager.hpp"
#include "triglav/desktop_ui/Splitter.hpp"
#include "triglav/desktop_ui/TabView.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/desktop_ui/TreeView.hpp"
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
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/ScrollBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <spdlog/spdlog.h>

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;
using triglav::desktop::WindowAttribute;
using triglav::desktop_ui::Dialog;
using triglav::desktop_ui::PopupManager;
using triglav::desktop_ui::TREE_ROOT;
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

class ColorChanger
{
 public:
   using Self = ColorChanger;

   ColorChanger(triglav::ui_core::RectBox& rectBox, triglav::desktop_ui::MenuController& controller) :
       m_rectBox(rectBox),
       TG_CONNECT(controller, OnClicked, on_clicked)
   {
   }

   void on_clicked(const triglav::Name name, const triglav::desktop_ui::MenuItem& /*item*/) const
   {
      switch (name) {
      case "green"_name:
         m_rectBox.set_color({0, 1, 0, 1});
         break;
      case "red"_name:
         m_rectBox.set_color({1, 0, 0, 1});
         break;
      case "blue"_name:
         m_rectBox.set_color({0, 0, 1, 1});
         break;
      case "yellow"_name:
         m_rectBox.set_color({1, 1, 0, 1});
         break;
      case "purple"_name:
         m_rectBox.set_color({0.5, 0, 0.5, 1});
         break;
      default:
         break;
      }
   }

   triglav::ui_core::RectBox& m_rectBox;
   TG_SINK(triglav::desktop_ui::MenuController, OnClicked);
};

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

   auto rootDialog = std::make_unique<Dialog>(instance, *device, display, glyphCache, resourceManager,
                                              triglav::Vector2i{initialWidth, initialHeight}, "Desktop UI Example"_strv);

   PopupManager dialogManager(instance, *device, glyphCache, resourceManager, rootDialog->surface());

   triglav::desktop_ui::DesktopUIManager desktopUiManager(triglav::desktop_ui::ThemeProperties::get_default(), rootDialog->surface(),
                                                          dialogManager);

   auto& globalVerLayout = rootDialog->create_root_widget<triglav::ui_core::VerticalLayout>({
      .padding = {0.0f, 0.0f, 0.0f, 0.0f},
      .separation = 0.0f,
   });

   triglav::desktop_ui::MenuController menu_bar_controller;
   menu_bar_controller.add_submenu("file"_name, "File"_strv);
   menu_bar_controller.add_subitem("file"_name, "file.open"_name, "Open"_strv);
   menu_bar_controller.add_subitem("file"_name, "file.save"_name, "Save"_strv);
   menu_bar_controller.add_submenu("file"_name, "file.recent_files"_name, "Recent Files"_strv);
   menu_bar_controller.add_subitem("file.recent_files"_name, "file.recent_files.a"_name, "foo.txt"_strv);
   menu_bar_controller.add_subitem("file.recent_files"_name, "file.recent_files.b"_name, "bar.txt"_strv);
   menu_bar_controller.add_subitem("file.recent_files"_name, "file.recent_files.c"_name, "dar.txt"_strv);

   menu_bar_controller.add_subitem("file"_name, "file.close"_name, "Close"_strv);

   menu_bar_controller.add_submenu("edit"_name, "Edit"_strv);
   menu_bar_controller.add_subitem("edit"_name, "edit.undo"_name, "Undo"_strv);
   menu_bar_controller.add_subitem("edit"_name, "edit.redo"_name, "Redo"_strv);

   menu_bar_controller.add_submenu("view"_name, "View"_strv);
   menu_bar_controller.add_subitem("view"_name, "view.font_increase"_name, "Increase Font"_strv);
   menu_bar_controller.add_subitem("view"_name, "view.font_decrease"_name, "Decrease Font"_strv);

   menu_bar_controller.add_submenu("help"_name, "Help"_strv);
   menu_bar_controller.add_subitem("help"_name, "help.online_help"_name, "Online Help"_strv);
   menu_bar_controller.add_subitem("help"_name, "help.about"_name, "About"_strv);

   globalVerLayout.create_child<triglav::desktop_ui::MenuBar>({
      .manager = &desktopUiManager,
      .controller = &menu_bar_controller,
   });

   auto& splitter = globalVerLayout.create_child<triglav::desktop_ui::Splitter>({
      .manager = &desktopUiManager,
      .offset = static_cast<float>(g_defaultWidth) / 2,
      .axis = triglav::ui_core::Axis::Horizontal,
   });

   auto& vertSplitter = splitter.create_preceding<triglav::desktop_ui::Splitter>({
      .manager = &desktopUiManager,
      .offset = static_cast<float>(g_defaultHeight) / 2,
      .axis = triglav::ui_core::Axis::Vertical,
   });

   vertSplitter.create_preceding<triglav::ui_core::EmptySpace>({
      .size = {200, 200},
   });

   triglav::desktop_ui::MenuController context_menu_controller;
   context_menu_controller.add_item("green"_name, "Make green"_strv);
   context_menu_controller.add_item("red"_name, "Make red"_strv);
   context_menu_controller.add_seperator();
   context_menu_controller.add_item("blue"_name, "Make blue"_strv);
   context_menu_controller.add_submenu("more"_name, "More colors..."_strv);
   context_menu_controller.add_subitem("more"_name, "yellow"_name, "Yellow"_strv);
   context_menu_controller.add_subitem("more"_name, "purple"_name, "Purple"_strv);

   auto& contextMenu = vertSplitter.create_following<triglav::desktop_ui::ContextMenu>({
      .manager = &desktopUiManager,
      .controller = &context_menu_controller,
   });

   auto& leftBox = contextMenu.create_content<triglav::ui_core::RectBox>({
      .color = {0.0f, 0.7f, 0.0f, 1.0f},
      .borderRadius = {10.0f, 10.0f, 10.0f, 10.0f},
      .borderColor = {0.0f, 0.8f, 0.0f, 1.0f},
      .borderWidth = 1.0f,
   });

   ColorChanger colorChanger(leftBox, context_menu_controller);

   leftBox.create_content<triglav::ui_core::EmptySpace>({
      .size = {200, 200},
   });

   auto& globalLayout = splitter.create_following<triglav::ui_core::VerticalLayout>({
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


   auto& tabView = scroll.create_content<triglav::desktop_ui::TabView>({
      .manager = &desktopUiManager,
      .tabNames = {"Random Items"_str, "Tree Example"_str},
   });

   auto& layout = tabView.create_child<triglav::ui_core::VerticalLayout>({
      .padding = {5.0f, 5.0f, 5.0f, 5.0f},
      .separation = 10.0f,
   });


   layout.create_child<triglav::desktop_ui::TextInput>({
      .manager = &desktopUiManager,
      .text = "Text Edit",
   });

   layout.create_child<triglav::desktop_ui::DropDownMenu>({
      .manager = &desktopUiManager,
      .items = {"Monday"_str, "Tuesday"_str, "Wednesday"_str, "Thursday"_str, "Friday"_str, "Saturday"_str, "Sunday"_str},
      .selectedItem = 0,
   });

   auto& checkBoxLayout = layout.create_child<triglav::ui_core::HorizontalLayout>({
      .padding = {2.0f, 2.0f, 2.0f, 2.0f},
      .separation = 8.0f,
   });

   triglav::desktop_ui::RadioGroup radioGroup;

   for (int i = 0; i < 4; ++i) {
      auto& checkBox = checkBoxLayout.create_child<triglav::desktop_ui::CheckBox>({
         .manager = &desktopUiManager,
         .radioGroup = &radioGroup,
         .isEnabled = false,
      });
      checkBox.create_content<triglav::ui_core::Image>({
         .texture = "texture/ui_atlas.tex"_rc,
         .maxSize = triglav::Vector2{16, 16},
         .region = triglav::Vector4{i * 64, 0, 64, 64},
      });
      radioGroup.add_check_box(&checkBox);
   }

   layout.create_child<triglav::desktop_ui::Button>({
      .manager = &desktopUiManager,
      .label = "Example",
   });

   layout.create_child<triglav::ui_core::Image>({
      .texture = "texture/ui_images.tex"_rc,
      .maxSize = triglav::Vector2{200, 200},
   });

   triglav::desktop_ui::TreeController controller;

   auto container_item = controller.add_item(TREE_ROOT, triglav::desktop_ui::TreeItem{
                                                           .iconName = "texture/ui_images.tex"_rc,
                                                           .iconRegion = {0, 0, 200, 200},
                                                           .label = "Collection",
                                                           .hasChildren = true,
                                                        });

   for (int i = 0; i < 2; ++i) {
      controller.add_item(container_item, triglav::desktop_ui::TreeItem{
                                             .iconName = "texture/ui_images.tex"_rc,
                                             .iconRegion = {0, 0, 200, 200},
                                             .label = triglav::format("Sub Item {}", i),
                                             .hasChildren = false,
                                          });
   }

   const auto child_item = controller.add_item(container_item, triglav::desktop_ui::TreeItem{
                                                                  .iconName = "texture/ui_images.tex"_rc,
                                                                  .iconRegion = {0, 0, 200, 200},
                                                                  .label = "Child Collection",
                                                                  .hasChildren = true,
                                                               });

   for (int i = 0; i < 4; ++i) {
      controller.add_item(child_item, triglav::desktop_ui::TreeItem{
                                         .iconName = "texture/ui_images.tex"_rc,
                                         .iconRegion = {0, 0, 200, 200},
                                         .label = triglav::format("Sub sub Item {}", i),
                                         .hasChildren = false,
                                      });
   }

   for (int i = 2; i < 4; ++i) {
      controller.add_item(container_item, triglav::desktop_ui::TreeItem{
                                             .iconName = "texture/ui_images.tex"_rc,
                                             .iconRegion = {0, 0, 200, 200},
                                             .label = triglav::format("Sub Item {}", i),
                                             .hasChildren = false,
                                          });
   }

   for (int i = 0; i < 4; ++i) {
      controller.add_item(TREE_ROOT, triglav::desktop_ui::TreeItem{
                                        .iconName = "texture/ui_images.tex"_rc,
                                        .iconRegion = {0, 0, 200, 200},
                                        .label = triglav::format("Test Item {}", i),
                                        .hasChildren = false,
                                     });
   }

   tabView.create_child<triglav::desktop_ui::TreeView>({
      .manager = &desktopUiManager,
      .controller = &controller,
      .extended_items = {container_item},
   });

   rootDialog->initialize();

   while (!rootDialog->should_close()) {
      dialogManager.tick();
      rootDialog->update();
      display.dispatch_messages();

      triglav::threading::Scheduler::the().tick();
   }

   device->await_all();

   ThreadPool::the().quit();

   return EXIT_SUCCESS;
}
