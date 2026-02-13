
#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/io/Logging.hpp"
#include "triglav/project/Name.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/testing_core/GTest.hpp"
#include "triglav/testing_render_util/RenderSupport.hpp"
#include "triglav/threading/ThreadPool.hpp"

#include <print>

TG_PROJECT_NAME(triglav_render_core_test)

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;
using triglav::desktop::WindowAttribute;
using triglav::resource::ResourceManager;

using namespace triglav::string_literals;
using namespace triglav::name_literals;

TG_DEFINE_AWAITER(LoadAssetsAwaiter, ResourceManager, OnLoadedAssets)

int triglav_main(InputArgs& args, IDisplay& display)
{
   using namespace triglav::io::path_literals;

   // Initialize logger
   triglav::LogManager::the().register_listener<triglav::io::StreamLogger>(triglav::io::stdout_writer());

   auto instance = GAPI_CHECK(triglav::graphics_api::Instance::create_instance(&display));

   triglav::threading::ThreadPool::the().initialize(4);

   auto device = GAPI_CHECK(instance.create_device(nullptr, triglav::graphics_api::DevicePickStrategy::PreferDedicated,
                                                   triglav::graphics_api::DeviceFeature::RayTracing));

   triglav::testing_render_util::RenderSupport::the().m_device = device.get();

   triglav::font::FontManger font_manager;
   ResourceManager resource_manager(*device, font_manager);
   triglav::testing_render_util::RenderSupport::the().m_resource_manager = &resource_manager;

   triglav::testing_render_util::RenderSupport::the().initialize_render_doc();

   LoadAssetsAwaiter awaiter(resource_manager);

   resource_manager.load_asset_list(triglav::project::PathManager::the().translate_path("engine/index.yaml"_rc));

   std::println("Loading resources");

   awaiter.await();

   std::println("Finished loading resources");

   int status = 0;

   testing::InitGoogleTest(&args.arg_count, const_cast<char**>(args.args));
   status = RUN_ALL_TESTS();

   triglav::testing_render_util::RenderSupport::the().on_quit();
   triglav::threading::ThreadPool::the().quit();

   return status;
}
