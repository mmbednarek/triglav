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

class EventListener final : public triglav::desktop::DefaultSurfaceEventListener
{
 public:
   void on_close() override
   {
      this->shouldClose = true;
   }

   bool shouldClose = false;
};

class Awaiter
{
 public:
   using Self = Awaiter;

   Awaiter(ResourceManager& resourceManager) :
       sink_OnLoadedAssets(resourceManager.OnLoadedAssets.connect<&Awaiter::on_loaded_assets>(this))
   {
   }

   void on_loaded_assets()
   {
      {
         std::lock_guard guard(m_mutex);
         m_ready = true;
      }
      m_cond.notify_one();
   }

   void await()
   {
      std::unique_lock lock(m_mutex);
      m_cond.wait(lock, [this] { return m_ready; });
   }

 private:
   std::mutex m_mutex;
   std::condition_variable m_cond;
   bool m_ready = false;

   ResourceManager::OnLoadedAssetsDel::Sink<Self> sink_OnLoadedAssets;
};

int triglav_main(InputArgs& args, IDisplay& display)
{
   auto surface = display.create_surface(400, 400, WindowAttribute::Default);
   auto instance = GAPI_CHECK(triglav::graphics_api::Instance::create_instance());
   auto gapiSurface = GAPI_CHECK(instance.create_surface(*surface));

   triglav::threading::ThreadPool::the().initialize(4);

   auto device = GAPI_CHECK(instance.create_device(gapiSurface, triglav::graphics_api::DevicePickStrategy::PreferDedicated,
                                                   triglav::graphics_api::DeviceFeature::None));

   triglav::test::TestingSupport::the().m_device = device.get();

   triglav::font::FontManger fontManager;
   ResourceManager resourceManager(*device, fontManager);
   triglav::test::TestingSupport::the().m_resourceManager = &resourceManager;

   Awaiter awaiter(resourceManager);

   resourceManager.load_asset_list(triglav::io::Path("content/index.yaml"));

   fmt::print("Loading resources\n");

   awaiter.await();

   fmt::print("Finished loading resources\n");

   EventListener listener;
   surface->add_event_listener(&listener);

   int status = 0;

   while (!listener.shouldClose) {
      display.dispatch_messages();

      testing::InitGoogleTest(&args.arg_count, const_cast<char**>(args.args));
      status = RUN_ALL_TESTS();

      listener.shouldClose = true;
   }

   triglav::threading::ThreadPool::the().quit();

   return status;
}
