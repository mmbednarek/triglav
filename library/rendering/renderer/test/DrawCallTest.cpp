#include "triglav/desktop/Desktop.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/renderer/BindlessScene.hpp"
#include "triglav/testing_core/GTest.hpp"
#include "triglav/testing_render_util/RenderSupport.hpp"

using triglav::Matrix4x4;
using triglav::MemorySize;
using triglav::Transform3D;
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

const std::array<Transform3D, SCENE_MESH_COUNT> SCENE_TRANSFORMS{
   Transform3D{
      .rotation = {std::sqrt(2.0f) / 2.0f, 0.5, 0.5, 0},
      .scale = {2, 3, 0.5f},
      .translation = {5, -5, 10},
   },
   Transform3D{
      .rotation = {std::sqrt(2.0f) / 2.0f, 0.5, 0.5, 0},
      .scale = {2, 3, 0.5f},
      .translation = {5, -5, 10},
   },
   Transform3D{
      .rotation = {0.271f, 0.653f, 0.271f, 0.653f},
      .scale = {10, 1, 1},
      .translation = {0, -10, 0},
   },
};

const std::array<Matrix4x4, SCENE_MESH_COUNT> SCENE_MATRICES{
   SCENE_TRANSFORMS[0].to_matrix(),
   SCENE_TRANSFORMS[1].to_matrix(),
   SCENE_TRANSFORMS[2].to_matrix(),
};

const std::array<BindlessSceneObject, SCENE_MESH_COUNT> SCENE_OBJECTS{
   BindlessSceneObject{
      .index_count = 64,
      .instance_count = 1,
      .index_offset = 0,
      .vertex_offset = 0,
      .instance_offset = 0,
      .material_id = 0,
      .transform_id = 0,
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
      .material_id = 0,
      .transform_id = 1,
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
      .material_id = 0,
      .transform_id = 2,
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
      .material_id = 0,
      .transform = SCENE_MATRICES[0],
      .normal_transform = SCENE_TRANSFORMS[0].to_normal_matrix(),
   },
   DrawCall{
      .index_count = 32,
      .instance_count = 2,
      .index_offset = 1,
      .vertex_offset = 2,
      .instance_offset = 3,
      .material_id = 0,
      .transform = SCENE_MATRICES[1],
      .normal_transform = SCENE_TRANSFORMS[1].to_normal_matrix(),
   },
   DrawCall{
      .index_count = 20,
      .instance_count = 1,
      .index_offset = 0,
      .vertex_offset = 100,
      .instance_offset = 200,
      .material_id = 0,
      .transform = SCENE_MATRICES[2],
      .normal_transform = SCENE_TRANSFORMS[2].to_normal_matrix(),
   },
};

TEST(DrawCallTest, Passthrough)
{
   RenderTestState state;

   state.bctx.declare_staging_buffer("scene_upload"_name, 16 * sizeof(BindlessSceneObject));
   state.bctx.declare_staging_buffer("draw_call_fetch"_name, 16 * sizeof(DrawCall));
   state.bctx.declare_staging_buffer("matrices_upload"_name, 16 * sizeof(Matrix4x4));

   state.bctx.declare_buffer("scene_meshes"_name, 16 * sizeof(BindlessSceneObject));
   state.bctx.declare_buffer("matrices"_name, 16 * sizeof(Matrix4x4));
   state.bctx.declare_buffer("draw_calls_base"_name, 16 * sizeof(DrawCall));
   state.bctx.declare_buffer("draw_calls_normal_map"_name, 16 * sizeof(DrawCall));
   state.bctx.init_buffer("mesh_count"_name, SCENE_MESH_COUNT);

   state.bctx.copy_buffer("scene_upload"_name, "scene_meshes"_name);
   state.bctx.copy_buffer("matrices_upload"_name, "matrices"_name);

   state.bctx.bind_compute_shader("shader/bindless_geometry/passthrough.cshader"_rc);

   state.bctx.bind_storage_buffer(0, "scene_meshes"_name);
   state.bctx.bind_uniform_buffer(1, "mesh_count"_name);
   state.bctx.bind_storage_buffer(2, "matrices"_name);
   state.bctx.bind_storage_buffer(3, "draw_calls_base"_name);
   state.bctx.bind_storage_buffer(4, "draw_calls_normal_map"_name);

   state.bctx.dispatch({1, 1, 1});

   state.bctx.copy_buffer("draw_calls_base"_name, "draw_call_fetch"_name);

   state.build();

   state.map_buffer("matrices_upload"_name,
                    [](void* buffer) { std::memcpy(buffer, SCENE_MATRICES.data(), sizeof(Matrix4x4) * SCENE_MATRICES.size()); });
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
   uint32_t channel_type;
};

struct AnimationState
{
   float current_time;
   uint32_t channel_count;
};

std::array<float, 5> TIMESTAMPS = {
   0.0f, 100.0f, 150.0f, 0.0f, 200.0f,
};

std::array<Vector4, 5> KEYFRAMES = {
   Vector4{-1.0f, -1.0f, 0.0f, 0.0f}, Vector4{1.0f, -1.0f, 0.0f, 0.0f}, Vector4{1.0f, 1.0f, 0.0f, 0.0f},
   Vector4{0.0f, 0.0f, 10.0f, 0.0f},  Vector4{0.0f, 0.0f, 20.0f, 0.0f},
};

TEST(DrawCallTest, AnimationTest)
{
   RenderTestState state;

   state.bctx.declare_staging_buffer("transform_stage"_name, 16 * sizeof(Transform3D));
   state.bctx.declare_staging_buffer("keyframes_upload"_name, 16 * sizeof(Vector4));
   state.bctx.declare_staging_buffer("timestamp_upload"_name, 16 * sizeof(float));

   state.bctx.declare_buffer("transform"_name, 16 * sizeof(Transform3D));
   state.bctx.declare_buffer("keyframes"_name, 16 * sizeof(Vector4));
   state.bctx.declare_buffer("timestamp"_name, 16 * sizeof(float));

   state.bctx.init_buffer("animation_state"_name, AnimationState{100.0f, 2});
   state.bctx.init_buffer("channel_states"_name, std::array{ChannelState{
                                                               .start_time = 50.0f,
                                                               .target_mesh = 1,
                                                               .last_keyframe = 2,
                                                               .target_keyframe = 1,
                                                               .channel_type = 0,
                                                            },
                                                            ChannelState{
                                                               .start_time = 20.0f,
                                                               .target_mesh = 2,
                                                               .last_keyframe = 4,
                                                               .target_keyframe = 4,
                                                               .channel_type = 0,
                                                            }});

   state.bctx.copy_buffer("transform_stage"_name, "transform"_name);
   state.bctx.copy_buffer("keyframes_upload"_name, "keyframes"_name);
   state.bctx.copy_buffer("timestamp_upload"_name, "timestamp"_name);

   state.bctx.bind_compute_shader("shader/bindless_geometry/animation.cshader"_rc);

   state.bctx.bind_storage_buffer(0, "keyframes"_name);
   state.bctx.bind_storage_buffer(1, "timestamp"_name);
   state.bctx.bind_storage_buffer(2, "channel_states"_name);
   state.bctx.bind_uniform_buffer(3, "animation_state"_name);
   state.bctx.bind_storage_buffer(4, "transform"_name);
   state.bctx.dispatch({1, 1, 1});

   state.bctx.copy_buffer("transform"_name, "transform_stage"_name);

   state.build();

   state.map_buffer("transform_stage"_name,
                    [](void* buffer) { std::memcpy(buffer, SCENE_TRANSFORMS.data(), sizeof(Transform3D) * SCENE_TRANSFORMS.size()); });
   state.map_buffer("keyframes_upload"_name,
                    [](void* buffer) { std::memcpy(buffer, KEYFRAMES.data(), sizeof(Vector4) * KEYFRAMES.size()); });
   state.map_buffer("timestamp_upload"_name,
                    [](void* buffer) { std::memcpy(buffer, TIMESTAMPS.data(), sizeof(float) * TIMESTAMPS.size()); });

   state.execute_and_await();

   state.map_buffer("transform_stage"_name, [](void* buffer) {
      const auto& got = *static_cast<std::array<Transform3D, 3>*>(buffer);
      ASSERT_EQ(got[0].translation, SCENE_TRANSFORMS[0].translation);
      triglav::Vector3 expected1{0.0f, -1.0f, 0.0f};
      ASSERT_EQ(got[1].translation, expected1);
      triglav::Vector3 expected2{0.0f, 0.0f, 14.0f};
      ASSERT_EQ(got[2].translation, expected2);
   });
}


constexpr uint MAX_MATRICES = 6;

struct MatrixMulTask
{
   uint destination_id;
   uint matrix_count;
   uint matrix_ids[MAX_MATRICES];
};

const std::array<MatrixMulTask, 2> MATRIX_MUL_TASKS{MatrixMulTask{.destination_id = 3, .matrix_count = 2, .matrix_ids = {1, 2}},
                                                    MatrixMulTask{.destination_id = 4, .matrix_count = 3, .matrix_ids = {2, 0, 1}}};

const std::array<Matrix4x4, 5> MATRICES{
   SCENE_TRANSFORMS[0].to_matrix(), SCENE_TRANSFORMS[1].to_matrix(), SCENE_TRANSFORMS[2].to_matrix(), Matrix4x4{}, Matrix4x4{},
};

TEST(DrawCallTest, MatrixMulTest)
{
   RenderTestState state;

   state.bctx.declare_staging_buffer("task_stage"_name, 16 * sizeof(MatrixMulTask));
   state.bctx.declare_staging_buffer("matrix_stage"_name, 16 * sizeof(Matrix4x4));
   state.bctx.declare_buffer("task"_name, 16 * sizeof(MatrixMulTask));
   state.bctx.declare_buffer("matrix"_name, 16 * sizeof(Matrix4x4));

   state.bctx.copy_buffer("task_stage"_name, "task"_name);
   state.bctx.copy_buffer("matrix_stage"_name, "matrix"_name);
   state.bctx.init_buffer("count"_name, static_cast<int>(MATRIX_MUL_TASKS.size()));

   state.bctx.bind_compute_shader("shader/bindless_geometry/matrix_multiply.cshader"_rc);

   state.bctx.bind_storage_buffer(0, "task"_name);
   state.bctx.bind_uniform_buffer(1, "count"_name);
   state.bctx.bind_storage_buffer(2, "matrix"_name);

   state.bctx.dispatch({1, 1, 1});

   state.bctx.copy_buffer("matrix"_name, "matrix_stage"_name);

   state.build();

   state.map_buffer("task_stage"_name,
                    [&](void* buffer) { std::memcpy(buffer, MATRIX_MUL_TASKS.data(), sizeof(MatrixMulTask) * MATRIX_MUL_TASKS.size()); });
   state.map_buffer("matrix_stage"_name, [&](void* buffer) { std::memcpy(buffer, MATRICES.data(), sizeof(Matrix4x4) * MATRICES.size()); });

   state.execute_and_await();

   state.map_buffer("matrix_stage"_name, [&](void* buffer) {
      const auto* matrices = static_cast<const Matrix4x4*>(buffer);

      const auto expected_mat1 = MATRICES[1] * MATRICES[2];
      compare_matrices(matrices[3], expected_mat1);
      const auto expected_mat2 = MATRICES[2] * MATRICES[0] * MATRICES[1];
      compare_matrices(matrices[4], expected_mat2);
   });
}
