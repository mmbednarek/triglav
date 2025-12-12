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

constexpr auto g_default_width = 800;
constexpr auto g_default_height = 600;

TG_DEFINE_AWAITER(ResourceLoadedAwaiter, ResourceManager, OnLoadedAssets)

class ColorChanger
{
 public:
   using Self = ColorChanger;

   ColorChanger(triglav::ui_core::RectBox& rect_box, triglav::desktop_ui::MenuController& controller) :
       m_rect_box(rect_box),
       TG_CONNECT(controller, OnClicked, on_clicked)
   {
   }

   void on_clicked(const triglav::Name name, const triglav::desktop_ui::MenuItem& /*item*/) const
   {
      switch (name) {
      case "green"_name:
         m_rect_box.set_color({0, 1, 0, 1});
         break;
      case "red"_name:
         m_rect_box.set_color({1, 0, 0, 1});
         break;
      case "blue"_name:
         m_rect_box.set_color({0, 0, 1, 1});
         break;
      case "yellow"_name:
         m_rect_box.set_color({1, 1, 0, 1});
         break;
      case "purple"_name:
         m_rect_box.set_color({0.5, 0, 0.5, 1});
         break;
      default:
         break;
      }
   }

   triglav::ui_core::RectBox& m_rect_box;
   TG_SINK(triglav::desktop_ui::MenuController, OnClicked);
};

int triglav_main(InputArgs& args, IDisplay& display)
{
   // Parse program arguments
   CommandLine::the().parse(args.arg_count, args.args);

   // Assign ID to the main thread
   triglav::threading::set_thread_id(triglav::threading::g_main_thread);

   // Initialize global thread pool
   const auto thread_count = CommandLine::the().arg_int("threadCount"_name).value_or(8);
   ThreadPool::the().initialize(std::clamp(thread_count, 0, 8));

   const auto instance = GAPI_CHECK(triglav::graphics_api::Instance::create_instance(&display));

   std::unique_ptr<triglav::graphics_api::Device> device;

   {
      const auto surface = display.create_surface("Temporary Window"_strv, {32, 32}, WindowAttribute::Default);
      const auto surface_gfx = GAPI_CHECK(instance.create_surface(*surface));

      device = GAPI_CHECK(instance.create_device(&surface_gfx, triglav::graphics_api::DevicePickStrategy::PreferDedicated,
                                                 triglav::graphics_api::DeviceFeature::None));
   }

   FontManger font_manager;
   ResourceManager resource_manager(*device, font_manager);

   GlyphCache glyph_cache(*device, resource_manager);

   ResourceLoadedAwaiter resource_awaiter(resource_manager);
   resource_manager.load_asset_list(PathManager::the().content_path().sub("index_base.yaml"));
   resource_awaiter.await();

   const auto initial_width = static_cast<triglav::u32>(CommandLine::the().arg_int("width"_name).value_or(g_default_width));
   const auto initial_height = static_cast<triglav::u32>(CommandLine::the().arg_int("height"_name).value_or(g_default_height));

   auto root_dialog = std::make_unique<Dialog>(instance, *device, display, glyph_cache, resource_manager,
                                               triglav::Vector2i{initial_width, initial_height}, "Desktop UI Example"_strv);

   PopupManager dialog_manager(instance, *device, glyph_cache, resource_manager, root_dialog->surface());

   triglav::desktop_ui::DesktopUIManager desktop_ui_manager(triglav::desktop_ui::ThemeProperties::get_default(), root_dialog->surface(),
                                                            dialog_manager);

   auto& global_ver_layout = root_dialog->create_root_widget<triglav::ui_core::VerticalLayout>({
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

   global_ver_layout.create_child<triglav::desktop_ui::MenuBar>({
      .manager = &desktop_ui_manager,
      .controller = &menu_bar_controller,
   });

   auto& splitter = global_ver_layout.create_child<triglav::desktop_ui::Splitter>({
      .manager = &desktop_ui_manager,
      .offset = static_cast<float>(g_default_width) / 2,
      .axis = triglav::ui_core::Axis::Horizontal,
   });

   auto& vert_splitter = splitter.create_preceding<triglav::desktop_ui::Splitter>({
      .manager = &desktop_ui_manager,
      .offset = static_cast<float>(g_default_height) / 2,
      .axis = triglav::ui_core::Axis::Vertical,
   });

   vert_splitter.create_preceding<triglav::ui_core::EmptySpace>({
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

   auto& context_menu = vert_splitter.create_following<triglav::desktop_ui::ContextMenu>({
      .manager = &desktop_ui_manager,
      .controller = &context_menu_controller,
   });

   auto& left_box = context_menu.create_content<triglav::ui_core::RectBox>({
      .color = {0.0f, 0.7f, 0.0f, 1.0f},
      .border_radius = {10.0f, 10.0f, 10.0f, 10.0f},
      .border_color = {0.0f, 0.8f, 0.0f, 1.0f},
      .border_width = 1.0f,
   });

   ColorChanger color_changer(left_box, context_menu_controller);

   left_box.create_content<triglav::ui_core::EmptySpace>({
      .size = {200, 200},
   });

   auto& global_layout = splitter.create_following<triglav::ui_core::VerticalLayout>({
      .padding = {0, 0, 0, 0},
      .separation = 0.0f,
   });

   auto& rect = global_layout.create_child<triglav::ui_core::RectBox>({
      .color = {1, 0, 1, 1},
      .border_radius = {0, 0, 0, 0},
      .border_color = {0, 0, 0, 0},
      .border_width = 0,
   });
   rect.create_content<triglav::ui_core::EmptySpace>({.size = {200, 200}});

   auto& scroll = global_layout.create_child<triglav::ui_core::ScrollBox>({.max_height = 300.0f});

   auto& rect2 = global_layout.create_child<triglav::ui_core::RectBox>({
      .color = {1, 0, 1, 1},
      .border_radius = {0, 0, 0, 0},
      .border_color = {0, 0, 0, 0},
      .border_width = 0,
   });
   rect2.create_content<triglav::ui_core::EmptySpace>({.size = {200, 200}});


   auto& tab_view = scroll.create_content<triglav::desktop_ui::TabView>({
      .manager = &desktop_ui_manager,
      .tab_names = {"Random Items"_str, "Tree Example"_str},
   });

   auto& layout = tab_view.create_child<triglav::ui_core::VerticalLayout>({
      .padding = {5.0f, 5.0f, 5.0f, 5.0f},
      .separation = 10.0f,
   });


   layout.create_child<triglav::desktop_ui::TextInput>({
      .manager = &desktop_ui_manager,
      .text = "Text Edit",
   });

   layout.create_child<triglav::desktop_ui::DropDownMenu>({
      .manager = &desktop_ui_manager,
      .items = {"Monday"_str, "Tuesday"_str, "Wednesday"_str, "Thursday"_str, "Friday"_str, "Saturday"_str, "Sunday"_str},
      .selected_item = 0,
   });

   auto& check_box_layout = layout.create_child<triglav::ui_core::HorizontalLayout>({
      .padding = {2.0f, 2.0f, 2.0f, 2.0f},
      .separation = 8.0f,
   });

   triglav::desktop_ui::RadioGroup radio_group;

   for (int i = 0; i < 4; ++i) {
      auto& check_box = check_box_layout.create_child<triglav::desktop_ui::CheckBox>({
         .manager = &desktop_ui_manager,
         .radio_group = &radio_group,
         .is_enabled = false,
      });
      check_box.create_content<triglav::ui_core::Image>({
         .texture = "texture/ui_atlas.tex"_rc,
         .max_size = triglav::Vector2{16, 16},
         .region = triglav::Vector4{i * 64, 0, 64, 64},
      });
      radio_group.add_check_box(&check_box);
   }

   auto& btn = layout.create_child<triglav::desktop_ui::Button>({
      .manager = &desktop_ui_manager,
   });
   btn.create_content<triglav::ui_core::TextBox>({
      .font_size = 10,
      .typeface = desktop_ui_manager.properties().base_typeface,
      .content = "Click Me!",
      .color = triglav::palette::WHITE,
      .horizontal_alignment = HorizontalAlignment::Center,
      .vertical_alignment = VerticalAlignment::Center,
   });

   layout.create_child<triglav::ui_core::Image>({
      .texture = "texture/ui_images.tex"_rc,
      .max_size = triglav::Vector2{200, 200},
   });

   triglav::desktop_ui::TreeController controller;

   auto container_item = controller.add_item(TREE_ROOT, triglav::desktop_ui::TreeItem{
                                                           .icon_name = "texture/ui_images.tex"_rc,
                                                           .icon_region = {0, 0, 200, 200},
                                                           .label = "Collection",
                                                           .has_children = true,
                                                        });

   for (int i = 0; i < 2; ++i) {
      controller.add_item(container_item, triglav::desktop_ui::TreeItem{
                                             .icon_name = "texture/ui_images.tex"_rc,
                                             .icon_region = {0, 0, 200, 200},
                                             .label = triglav::format("Sub Item {}", i),
                                             .has_children = false,
                                          });
   }

   const auto child_item = controller.add_item(container_item, triglav::desktop_ui::TreeItem{
                                                                  .icon_name = "texture/ui_images.tex"_rc,
                                                                  .icon_region = {0, 0, 200, 200},
                                                                  .label = "Child Collection",
                                                                  .has_children = true,
                                                               });

   for (int i = 0; i < 4; ++i) {
      controller.add_item(child_item, triglav::desktop_ui::TreeItem{
                                         .icon_name = "texture/ui_images.tex"_rc,
                                         .icon_region = {0, 0, 200, 200},
                                         .label = triglav::format("Sub sub Item {}", i),
                                         .has_children = false,
                                      });
   }

   for (int i = 2; i < 4; ++i) {
      controller.add_item(container_item, triglav::desktop_ui::TreeItem{
                                             .icon_name = "texture/ui_images.tex"_rc,
                                             .icon_region = {0, 0, 200, 200},
                                             .label = triglav::format("Sub Item {}", i),
                                             .has_children = false,
                                          });
   }

   for (int i = 0; i < 4; ++i) {
      controller.add_item(TREE_ROOT, triglav::desktop_ui::TreeItem{
                                        .icon_name = "texture/ui_images.tex"_rc,
                                        .icon_region = {0, 0, 200, 200},
                                        .label = triglav::format("Test Item {}", i),
                                        .has_children = false,
                                     });
   }

   tab_view.create_child<triglav::desktop_ui::TreeView>({
      .manager = &desktop_ui_manager,
      .controller = &controller,
      .extended_items = {container_item},
   });

   root_dialog->initialize();

   while (!root_dialog->should_close()) {
      dialog_manager.tick();
      root_dialog->update();
      display.dispatch_messages();

      triglav::threading::Scheduler::the().tick();
   }

   device->await_all();

   ThreadPool::the().quit();

   return EXIT_SUCCESS;
}
