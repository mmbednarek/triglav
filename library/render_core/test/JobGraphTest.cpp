#include <gtest/gtest.h>

#include "TestingSupport.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/JobGraph.hpp"

#include <iostream>

using triglav::Vector2i;
using triglav::graphics_api::BufferAccess;
using triglav::graphics_api::BufferUsage;
using triglav::graphics_api::PipelineStage;
using triglav::render_core::JobGraph;
using triglav::render_core::PipelineCache;
using triglav::render_core::ResourceStorage;
using triglav::test::TestingSupport;

using namespace triglav::name_literals;
using namespace triglav::render_core::literals;

namespace {

template<typename T>
[[nodiscard]] T read_buffer(triglav::graphics_api::Buffer& buffer)
{
   const auto mem = GAPI_CHECK(buffer.map_memory());
   return *static_cast<T*>(*mem);
}

template<typename T>
void write_buffer(triglav::graphics_api::Buffer& buffer, T value)
{
   const auto mem = GAPI_CHECK(buffer.map_memory());
   *static_cast<T*>(*mem) = value;
}

}// namespace

TEST(JobGraphTest, BasicDependency)
{
   PipelineCache pipelineCache(TestingSupport::device(), TestingSupport::resource_manager());
   ResourceStorage storage;
   JobGraph graph(TestingSupport::device(), TestingSupport::resource_manager(), pipelineCache, storage, Vector2i{800, 600});

   auto& firstCtx = graph.add_job("basic_dependency.first"_name);
   auto& secondCtx = graph.add_job("basic_dependency.second"_name);
   auto& thirdCtx = graph.add_job("basic_dependency.third"_name);
   graph.add_dependency("basic_dependency.second"_name, "basic_dependency.first"_name);
   graph.add_dependency("basic_dependency.third"_name, "basic_dependency.second"_name);

   firstCtx.init_buffer("basic_dependency.data"_name, 5);
   firstCtx.export_buffer("basic_dependency.data"_name, PipelineStage::ComputeShader, BufferAccess::ShaderRead,
                          BufferUsage::TransferSrc | BufferUsage::StorageBuffer);

   secondCtx.bind_compute_shader("testing/increase_number.cshader"_rc);
   secondCtx.bind_storage_buffer(0, "basic_dependency.data"_external);
   secondCtx.dispatch({1, 1, 1});

   thirdCtx.declare_staging_buffer("basic_dependency.user"_name, sizeof(int));
   thirdCtx.copy_buffer("basic_dependency.data"_external, "basic_dependency.user"_name);

   graph.build_jobs("basic_dependency.third"_name);

   auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   for (int frameIndex : triglav::Range(0, triglav::render_core::FRAMES_IN_FLIGHT_COUNT)) {
      graph.execute("basic_dependency.third"_name, frameIndex, &fence);
      fence.await();
      ASSERT_EQ(read_buffer<int>(storage.buffer("basic_dependency.user"_name, frameIndex)), 6);
   }
}

TEST(JobGraphTest, BasicInterframeDependency)
{
   PipelineCache pipelineCache(TestingSupport::device(), TestingSupport::resource_manager());
   ResourceStorage storage;
   JobGraph graph(TestingSupport::device(), TestingSupport::resource_manager(), pipelineCache, storage, Vector2i{800, 600});

   auto& buildContext = graph.add_job("basic_interframe_dependency"_name);
   graph.add_dependency_to_previous_frame("basic_interframe_dependency"_name, "basic_interframe_dependency"_name);

   buildContext.declare_buffer("basic_interframe_dependency.data"_name, sizeof(int));
   buildContext.declare_staging_buffer("basic_interframe_dependency.user"_name, sizeof(int));
   buildContext.declare_flag("is_first_frame"_name);
   buildContext.declare_flag("is_last_frame"_name);

   buildContext.if_enabled("is_first_frame"_name);
   buildContext.fill_buffer("basic_interframe_dependency.data"_name, 5);
   buildContext.end_if();

   buildContext.if_disabled("is_first_frame"_name);
   buildContext.copy_buffer("basic_interframe_dependency.data"_last_frame, "basic_interframe_dependency.data"_name);
   buildContext.end_if();

   buildContext.bind_compute_shader("testing/increase_number.cshader"_rc);
   buildContext.bind_storage_buffer(0, "basic_interframe_dependency.data"_name);
   buildContext.dispatch({1, 1, 1});

   buildContext.if_enabled("is_last_frame"_name);
   buildContext.copy_buffer("basic_interframe_dependency.data"_name, "basic_interframe_dependency.user"_name);
   buildContext.end_if();

   graph.build_jobs("basic_interframe_dependency"_name);

   auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   graph.enable_flag("basic_interframe_dependency"_name, "is_first_frame"_name);
   graph.execute("basic_interframe_dependency"_name, 0, &fence);
   fence.await();

   graph.disable_flag("basic_interframe_dependency"_name, "is_first_frame"_name);

   for (const int _ : triglav::Range(0, 20)) {
      graph.execute("basic_interframe_dependency"_name, 1, nullptr);
      graph.execute("basic_interframe_dependency"_name, 2, nullptr);
      graph.execute("basic_interframe_dependency"_name, 0, &fence);
      fence.await();
   }

   graph.enable_flag("basic_interframe_dependency"_name, "is_last_frame"_name);
   graph.execute("basic_interframe_dependency"_name, 1, &fence);
   fence.await();

   ASSERT_EQ(read_buffer<int>(storage.buffer("basic_interframe_dependency.user"_name, 1)), 67);
}
