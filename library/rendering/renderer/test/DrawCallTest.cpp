#include "triglav/desktop/Desktop.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/renderer/BindlessScene.hpp"
#include "triglav/testing_core/GTest.hpp"
#include "triglav/testing_render_util/RenderSupport.hpp"

using triglav::Matrix4x4;
using triglav::MemorySize;
using triglav::u32;
using triglav::Vector4;
using triglav::render_core::BuildContext;
using triglav::render_core::PipelineCache;
using triglav::render_core::ResourceStorage;
using triglav::renderer::BindlessSceneObject;
using triglav::renderer::DrawCall;
using triglav::testing_render_util::RenderSupport;

constexpr triglav::Vector2i DefaultSize{800, 600};

using namespace triglav::name_literals;

constexpr float PRECISION = 0.05f;

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

struct RenderTestState
{
   BuildContext bctx;
   ResourceStorage storage;
   PipelineCache pcache;
   std::optional<triglav::render_core::Job> job;

   RenderTestState() :
       bctx(RenderSupport::device(), RenderSupport::resource_manager(), DefaultSize),
       storage(RenderSupport::device()),
       pcache(RenderSupport::device(), RenderSupport::resource_manager())
   {
   }

   void build()
   {
      job.emplace(bctx.build_job(pcache, storage));
   }

   template<typename TCb>
   auto map_buffer(const triglav::Name buffer_name, TCb callback)
   {
      auto& mapping = storage.buffer(buffer_name, 0);
      const auto mem_map = mapping.map_memory();
      ASSERT_TRUE(mem_map.has_value());
      return callback(**mem_map);
   }

   void execute_and_await() const
   {
      assert(job.has_value());
      const auto fence = GAPI_CHECK(RenderSupport::device().create_fence());
      fence.await();

      const triglav::graphics_api::SemaphoreArray empty_list;
      job->execute(0, empty_list, empty_list, &fence);

      fence.await();
   }
};

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

constexpr auto SCENE_MESH_COUNT = 3;

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
   BindlessSceneObject{
      .index_count = 20,
      .instance_count = 1,
      .index_offset = 0,
      .vertex_offset = 100,
      .instance_offset = 200,
      .material_id = 2,
      .transform =
         {
            .rotation = {0.271f, 0.653f, 0.271f, 0.653f},
            .scale = {10, 1, 1},
            .translation = {0, -10, 0},
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
      .index_count = 64,
      .instance_count = 1,
      .index_offset = 0,
      .vertex_offset = 0,
      .instance_offset = 0,
      .material_id = 1,
      .transform = SCENE_OBJECTS[0].transform.to_matrix(),
      .normal_transform = SCENE_OBJECTS[0].transform.to_normal_matrix(),
   },
   DrawCall{
      .index_count = 32,
      .instance_count = 2,
      .index_offset = 1,
      .vertex_offset = 2,
      .instance_offset = 3,
      .material_id = 4,
      .transform = SCENE_OBJECTS[1].transform.to_matrix(),
      .normal_transform = SCENE_OBJECTS[1].transform.to_normal_matrix(),
   },
   DrawCall{
      .index_count = 20,
      .instance_count = 1,
      .index_offset = 0,
      .vertex_offset = 100,
      .instance_offset = 200,
      .material_id = 2,
      .transform = SCENE_OBJECTS[2].transform.to_matrix(),
      .normal_transform = SCENE_OBJECTS[2].transform.to_normal_matrix(),
   },
};

TEST(DrawCallTest, Passthrough)
{
   RenderTestState state;

   state.bctx.declare_staging_buffer("scene_upload"_name, 16 * sizeof(BindlessSceneObject));
   state.bctx.declare_staging_buffer("draw_call_fetch"_name, 16 * sizeof(DrawCall));

   state.bctx.declare_buffer("scene_meshes"_name, 16 * sizeof(BindlessSceneObject));
   state.bctx.declare_buffer("draw_calls"_name, 16 * sizeof(DrawCall));
   state.bctx.init_buffer("mesh_count"_name, SCENE_MESH_COUNT);

   state.bctx.copy_buffer("scene_upload"_name, "scene_meshes"_name);

   state.bctx.bind_compute_shader("shader/bindless_geometry/passthrough.cshader"_rc);

   state.bctx.bind_storage_buffer(0, "scene_meshes"_name);
   state.bctx.bind_uniform_buffer(1, "mesh_count"_name);
   state.bctx.bind_uniform_buffer(2, "draw_calls"_name);

   state.bctx.dispatch({1, 1, 1});

   state.bctx.copy_buffer("draw_calls"_name, "draw_call_fetch"_name);

   state.build();

   state.map_buffer("scene_upload"_name,
                    [](void* buffer) { std::memcpy(buffer, SCENE_OBJECTS.data(), sizeof(BindlessSceneObject) * SCENE_OBJECTS.size()); });

   state.execute_and_await();

   state.map_buffer("draw_call_fetch"_name, [](void* draw_call) {
      const auto& arr = *static_cast<std::array<DrawCall, SCENE_MESH_COUNT>*>(draw_call);
      for (MemorySize i = 0; i < arr.size(); ++i) {
         auto& got = arr[i];
         auto& expected = EXPECTED_DRAW_CALLS[i];

         ASSERT_EQ(got.index_count, expected.index_count);
         ASSERT_EQ(got.instance_count, expected.instance_count);
         ASSERT_EQ(got.index_offset, expected.index_offset);
         ASSERT_EQ(got.vertex_offset, expected.vertex_offset);
         ASSERT_EQ(got.instance_offset, expected.instance_offset);
         ASSERT_EQ(got.material_id, expected.material_id);
         ASSERT_TRUE(compare_matrices(got.transform, expected.transform));
         ASSERT_TRUE(compare_matrices(got.normal_transform, expected.normal_transform));
      }
   });
}

struct ChannelState
{
   float start_time;
   uint32_t target_mesh;
   uint32_t last_keyframe;
   uint32_t target_keyframe;
};

struct AnimationState
{
   float current_time;
   uint32_t channel_count;
};

// w component used for timestamp
std::array<Vector4, 5> KEYFRAMES = {
   Vector4{-1.0f, -1.0f, 0.0f, 0.0f}, Vector4{1.0f, -1.0f, 0.0f, 100.0f}, Vector4{1.0f, 1.0f, 0.0f, 150.0f},
   Vector4{0.0f, 0.0f, 10.0f, 0.0f},  Vector4{0.0f, 0.0f, 20.0f, 200.0f},
};

TEST(DrawCallTest, AnimationTest)
{
   RenderTestState state;

   state.bctx.declare_staging_buffer("scene_stage"_name, 16 * sizeof(BindlessSceneObject));
   state.bctx.declare_staging_buffer("keyframes_upload"_name, 16 * sizeof(Vector4));

   state.bctx.declare_buffer("scene"_name, 16 * sizeof(BindlessSceneObject));
   state.bctx.declare_buffer("keyframes"_name, 16 * sizeof(Vector4));

   state.bctx.init_buffer("animation_state"_name, AnimationState{100.0f, 2});
   state.bctx.init_buffer("channel_states"_name, std::array{ChannelState{
                                                               .start_time = 50.0f,
                                                               .target_mesh = 1,
                                                               .last_keyframe = 2,
                                                               .target_keyframe = 1,
                                                            },
                                                            ChannelState{
                                                               .start_time = 20.0f,
                                                               .target_mesh = 2,
                                                               .last_keyframe = 4,
                                                               .target_keyframe = 4,
                                                            }});

   state.bctx.copy_buffer("scene_stage"_name, "scene"_name);
   state.bctx.copy_buffer("keyframes_upload"_name, "keyframes"_name);

   state.bctx.bind_compute_shader("shader/bindless_geometry/animation.cshader"_rc);

   state.bctx.bind_storage_buffer(0, "keyframes"_name);
   state.bctx.bind_storage_buffer(1, "channel_states"_name);
   state.bctx.bind_uniform_buffer(2, "animation_state"_name);
   state.bctx.bind_storage_buffer(3, "scene"_name);
   state.bctx.dispatch({1, 1, 1});

   state.bctx.copy_buffer("scene"_name, "scene_stage"_name);

   state.build();

   state.map_buffer("scene_stage"_name,
                    [](void* buffer) { std::memcpy(buffer, SCENE_OBJECTS.data(), sizeof(BindlessSceneObject) * SCENE_OBJECTS.size()); });
   state.map_buffer("keyframes_upload"_name,
                    [](void* buffer) { std::memcpy(buffer, KEYFRAMES.data(), sizeof(Vector4) * KEYFRAMES.size()); });

   state.execute_and_await();

   state.map_buffer("scene_stage"_name, [](void* buffer) {
      const auto& got = *static_cast<std::array<BindlessSceneObject, 3>*>(buffer);
      ASSERT_EQ(got[0].transform.translation, SCENE_OBJECTS[0].transform.translation);
      triglav::Vector3 expected1{0.0f, -1.0f, 0.0f};
      ASSERT_EQ(got[1].transform.translation, expected1);
      triglav::Vector3 expected2{0.0f, 0.0f, 14.0f};
      ASSERT_EQ(got[2].transform.translation, expected2);
   });
}
