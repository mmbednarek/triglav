#include "TestingSupport.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/test_util/GTest.hpp"

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
   PipelineCache pipeline_cache(TestingSupport::device(), TestingSupport::resource_manager());
   ResourceStorage storage(TestingSupport::device());
   JobGraph graph(TestingSupport::device(), TestingSupport::resource_manager(), pipeline_cache, storage, Vector2i{800, 600});

   auto& first_ctx = graph.add_job("basic_dependency.first"_name);
   auto& second_ctx = graph.add_job("basic_dependency.second"_name);
   auto& third_ctx = graph.add_job("basic_dependency.third"_name);
   graph.add_dependency("basic_dependency.second"_name, "basic_dependency.first"_name);
   graph.add_dependency("basic_dependency.third"_name, "basic_dependency.second"_name);

   first_ctx.init_buffer("basic_dependency.data"_name, 5);
   first_ctx.export_buffer("basic_dependency.data"_name, PipelineStage::ComputeShader, BufferAccess::ShaderRead,
                           BufferUsage::TransferSrc | BufferUsage::StorageBuffer);

   second_ctx.bind_compute_shader("testing/shader/increase_number.cshader"_rc);
   second_ctx.bind_storage_buffer(0, "basic_dependency.data"_external);
   second_ctx.dispatch({1, 1, 1});

   third_ctx.declare_staging_buffer("basic_dependency.user"_name, sizeof(int));
   third_ctx.copy_buffer("basic_dependency.data"_external, "basic_dependency.user"_name);

   graph.build_jobs("basic_dependency.third"_name);

   auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   for (int frame_index : triglav::Range(0, triglav::render_core::FRAMES_IN_FLIGHT_COUNT)) {
      graph.execute("basic_dependency.third"_name, frame_index, &fence);
      fence.await();
      ASSERT_EQ(read_buffer<int>(storage.buffer("basic_dependency.user"_name, frame_index)), 6);
   }
}

TEST(JobGraphTest, BasicInterframeDependency)
{
   PipelineCache pipeline_cache(TestingSupport::device(), TestingSupport::resource_manager());
   ResourceStorage storage(TestingSupport::device());
   JobGraph graph(TestingSupport::device(), TestingSupport::resource_manager(), pipeline_cache, storage, Vector2i{800, 600});

   auto& build_context = graph.add_job("basic_interframe_dependency"_name);
   graph.add_dependency_to_previous_frame("basic_interframe_dependency"_name, "basic_interframe_dependency"_name);

   build_context.declare_buffer("basic_interframe_dependency.data"_name, sizeof(int));
   build_context.declare_staging_buffer("basic_interframe_dependency.user"_name, sizeof(int));
   build_context.declare_flag("is_first_frame"_name);
   build_context.declare_flag("is_last_frame"_name);

   build_context.if_enabled("is_first_frame"_name);
   build_context.fill_buffer("basic_interframe_dependency.data"_name, 5);
   build_context.end_if();

   build_context.if_disabled("is_first_frame"_name);
   build_context.copy_buffer("basic_interframe_dependency.data"_last_frame, "basic_interframe_dependency.data"_name);
   build_context.end_if();

   build_context.bind_compute_shader("testing/shader/increase_number.cshader"_rc);
   build_context.bind_storage_buffer(0, "basic_interframe_dependency.data"_name);
   build_context.dispatch({1, 1, 1});

   build_context.if_enabled("is_last_frame"_name);
   build_context.copy_buffer("basic_interframe_dependency.data"_name, "basic_interframe_dependency.user"_name);
   build_context.end_if();

   graph.build_jobs("basic_interframe_dependency"_name);

   auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   graph.enable_flag("basic_interframe_dependency"_name, "is_first_frame"_name);
   graph.execute("basic_interframe_dependency"_name, 0, &fence);
   fence.await();

   graph.disable_flag("basic_interframe_dependency"_name, "is_first_frame"_name);

   for ([[maybe_unused]] const int _ : triglav::Range(0, 20)) {
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
