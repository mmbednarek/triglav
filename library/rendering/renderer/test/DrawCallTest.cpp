#include "triglav/render_core/BuildContext.hpp"
#include "triglav/renderer/BindlessScene.hpp"
#include "triglav/testing_core/GTest.hpp"
#include "triglav/testing_render_util/RenderSupport.hpp"

using triglav::Matrix4x4;
using triglav::MemorySize;
using triglav::u32;
using triglav::render_core::BuildContext;
using triglav::render_core::PipelineCache;
using triglav::render_core::ResourceStorage;
using triglav::renderer::BindlessSceneObject;
using triglav::renderer::DrawCall;
using triglav::testing_render_util::RenderSupport;

constexpr triglav::Vector2i DefaultSize{800, 600};

using namespace triglav::name_literals;

constexpr float PRECISION = 0.0001f;

bool compare_matrices(const Matrix4x4& lhs, const Matrix4x4& rhs)
{
   for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
         if (std::abs(lhs[i][j] - rhs[i][j]) > PRECISION) {
            return false;
         }
      }
   }
   return true;
}

namespace {

// void execute_build_context(BuildContext& build_context, ResourceStorage& storage)
// {
//    PipelineCache pipeline_cache(RenderSupport::device(), RenderSupport::resource_manager());
//
//    const auto job = build_context.build_job(pipeline_cache, storage);
//
//    const auto fence = GAPI_CHECK(triglav::testing_render_util::RenderSupport::device().create_fence());
//    fence.await();
//
//    const triglav::graphics_api::SemaphoreArray empty_list;
//    job.execute(0, empty_list, empty_list, &fence);
//
//    fence.await();
// }

}// namespace

constexpr auto SCENE_MESH_COUNT = 2;

const std::array<BindlessSceneObject, SCENE_MESH_COUNT> SCENE_OBJECTS{
   BindlessSceneObject{
      .index_count = 64,
      .instance_count = 1,
      .index_offset = 0,
      .vertex_offset = 0,
      .instance_offset = 0,
      .material_id = 1,
      .transform =
         {
            .rotation = {1, 0, 0, 0},
            .scale = {1, 1, 1},
            .translation = {0, 0, 0},
         },
      .bounding_box =
         {
            .min = {-1, -1, -1},
            .max = {1, 1, 1},
         },
   },
   BindlessSceneObject{
      .index_count = 32,
      .instance_count = 2,
      .index_offset = 1,
      .vertex_offset = 2,
      .instance_offset = 3,
      .material_id = 4,
      .transform =
         {
            .rotation = {std::sqrt(2.0f) / 2.0f, 0.5, 0.5, 0},
            .scale = {2, 3, 0.5f},
            .translation = {5, -5, 10},
         },
      .bounding_box =
         {
            .min = {-1, -1, -1},
            .max = {1, 1, 1},
         },
   },
};

const std::array<DrawCall, SCENE_MESH_COUNT> EXPECTED_DRAW_CALLS{
   DrawCall{
      .indexCount = 64,
      .instanceCount = 1,
      .indexOffset = 0,
      .vertexOffset = 0,
      .instanceOffset = 0,
      .materialID = 1,
      .transform = SCENE_OBJECTS[0].transform.to_matrix(),
      .normalTransform = SCENE_OBJECTS[0].transform.to_normal_matrix(),
   },
   DrawCall{
      .indexCount = 32,
      .instanceCount = 2,
      .indexOffset = 1,
      .vertexOffset = 2,
      .instanceOffset = 3,
      .materialID = 4,
      .transform = SCENE_OBJECTS[1].transform.to_matrix(),
      .normalTransform = SCENE_OBJECTS[1].transform.to_normal_matrix(),
   },
};

TEST(DrawCallTest, Passthrough)
{
   BuildContext bctx(RenderSupport::device(), RenderSupport::resource_manager(), DefaultSize);

   bctx.declare_staging_buffer("scene_upload"_name, 16 * sizeof(BindlessSceneObject));
   bctx.declare_staging_buffer("draw_call_fetch"_name, 16 * sizeof(DrawCall));

   bctx.declare_buffer("scene_meshes"_name, 16 * sizeof(BindlessSceneObject));
   bctx.declare_buffer("draw_calls"_name, 16 * sizeof(DrawCall));
   bctx.init_buffer("mesh_count"_name, SCENE_MESH_COUNT);

   bctx.copy_buffer("scene_upload"_name, "scene_meshes"_name);

   bctx.bind_compute_shader("shader/bindless_geometry/passthrough.cshader"_rc);

   bctx.bind_storage_buffer(0, "scene_meshes"_name);
   bctx.bind_uniform_buffer(1, "mesh_count"_name);
   bctx.bind_uniform_buffer(2, "draw_calls"_name);

   bctx.dispatch({1, 1, 1});

   bctx.copy_buffer("draw_calls"_name, "draw_call_fetch"_name);

   ResourceStorage storage(RenderSupport::device());
   PipelineCache pipeline_cache(RenderSupport::device(), RenderSupport::resource_manager());

   const auto job = bctx.build_job(pipeline_cache, storage);

   {
      auto& load_buffer = storage.buffer("scene_upload"_name, 0);
      const auto mem_map = load_buffer.map_memory();
      ASSERT_TRUE(mem_map.has_value());
      mem_map->write(SCENE_OBJECTS.data(), sizeof(BindlessSceneObject) * SCENE_OBJECTS.size());
   }

   const auto fence = GAPI_CHECK(RenderSupport::device().create_fence());
   fence.await();

   const triglav::graphics_api::SemaphoreArray empty_list;
   job.execute(0, empty_list, empty_list, &fence);

   fence.await();

   {
      auto& fetch_buffer = storage.buffer("draw_call_fetch"_name, 0);
      const auto mem_map = fetch_buffer.map_memory();
      ASSERT_TRUE(mem_map.has_value());

      const auto& arr = mem_map->cast<std::array<DrawCall, 2>>();
      for (MemorySize i = 0; i < arr.size(); ++i) {
         auto& got = arr[i];
         auto& expected = EXPECTED_DRAW_CALLS[i];

         ASSERT_EQ(got.indexCount, expected.indexCount);
         ASSERT_EQ(got.instanceCount, expected.instanceCount);
         ASSERT_EQ(got.indexOffset, expected.indexOffset);
         ASSERT_EQ(got.vertexOffset, expected.vertexOffset);
         ASSERT_EQ(got.instanceOffset, expected.instanceOffset);
         ASSERT_EQ(got.materialID, expected.materialID);
         ASSERT_TRUE(compare_matrices(got.transform, expected.transform));
         ASSERT_TRUE(compare_matrices(got.normalTransform, expected.normalTransform));
      }
   }
}
