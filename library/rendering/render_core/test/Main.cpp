#include "TestingSupport.hpp"

#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/io/Logging.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/test_util/GTest.hpp"
#include "triglav/threading/ThreadPool.hpp"

#include <print>

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;
using triglav::desktop::WindowAttribute;
using triglav::resource::ResourceManager;

using namespace triglav::string_literals;

TG_DEFINE_AWAITER(LoadAssetsAwaiter, ResourceManager, OnLoadedAssets)

int triglav_main(InputArgs& args, IDisplay& display)
{
   using namespace triglav::io::path_literals;

   // Initialize logger
   triglav::LogManager::the().register_listener<triglav::io::StreamLogger>(triglav::io::stdout_writer());

   auto surface = display.create_surface("Testing window"_strv, {400, 400}, WindowAttribute::Default);
   auto instance = GAPI_CHECK(triglav::graphics_api::Instance::create_instance(&display));

   triglav::threading::ThreadPool::the().initialize(4);

   auto device = GAPI_CHECK(instance.create_device(nullptr, triglav::graphics_api::DevicePickStrategy::PreferDedicated,
                                                   triglav::graphics_api::DeviceFeature::RayTracing));

   triglav::test::TestingSupport::the().m_device = device.get();

   triglav::font::FontManger font_manager;
   ResourceManager resource_manager(*device, font_manager);
   triglav::test::TestingSupport::the().m_resource_manager = &resource_manager;

   triglav::test::TestingSupport::the().initialize_render_doc();

   LoadAssetsAwaiter awaiter(resource_manager);

   resource_manager.load_asset_list("content/index.yaml"_path);

   std::println("Loading resources");

   awaiter.await();

   std::println("Finished loading resources");

   int status = 0;

   testing::InitGoogleTest(&args.arg_count, const_cast<char**>(args.args));
   status = RUN_ALL_TESTS();

   triglav::test::TestingSupport::the().on_quit();
   triglav::threading::ThreadPool::the().quit();

   return status;
}
