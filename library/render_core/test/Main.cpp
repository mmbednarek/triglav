#include "TestingSupport.hpp"

#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/threading/ThreadPool.hpp"

#include <fmt/core.h>
#include <gtest/gtest.h>

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;
using triglav::desktop::WindowAttribute;
using triglav::resource::ResourceManager;

TG_DEFINE_AWAITER(LoadAssetsAwaiter, ResourceManager, OnLoadedAssets)

int triglav_main(InputArgs& args, IDisplay& display)
{
   auto surface = display.create_surface(400, 400, WindowAttribute::Default);
   auto instance = GAPI_CHECK(triglav::graphics_api::Instance::create_instance());

   triglav::threading::ThreadPool::the().initialize(4);

   auto device = GAPI_CHECK(instance.create_device(nullptr, triglav::graphics_api::DevicePickStrategy::PreferDedicated,
                                                   triglav::graphics_api::DeviceFeature::RayTracing));

   triglav::test::TestingSupport::the().m_device = device.get();

   triglav::font::FontManger fontManager;
   ResourceManager resourceManager(*device, fontManager);
   triglav::test::TestingSupport::the().m_resourceManager = &resourceManager;

   triglav::test::TestingSupport::the().initialize_render_doc();

   LoadAssetsAwaiter awaiter(resourceManager);

   resourceManager.load_asset_list(triglav::io::Path("content/index.yaml"));

   fmt::print("Loading resources\n");

   awaiter.await();

   fmt::print("Finished loading resources\n");

   int status = 0;

   testing::InitGoogleTest(&args.arg_count, const_cast<char**>(args.args));
   status = RUN_ALL_TESTS();

   triglav::test::TestingSupport::the().on_quit();
   triglav::threading::ThreadPool::the().quit();

   return status;
}
