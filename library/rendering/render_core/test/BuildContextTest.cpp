#include "TestingSupport.hpp"

#define TG_RAY_TRACING_TESTS 1

#include "triglav/Math.hpp"
#include "triglav/Ranges.hpp"
#include "triglav/geometry/DebugMesh.hpp"
#include "triglav/graphics_api/DescriptorLayoutCache.hpp"
#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/graphics_api/ray_tracing/InstanceBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/ShaderBindingTable.hpp"
#include "triglav/io/File.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/test_util/GTest.hpp"

#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#ifdef TG_ENABLE_RENDERDOC
#include <renderdoc_app.h>
#endif

using triglav::u32;
using triglav::render_core::BufferRef;
using triglav::render_core::BuildContext;
using triglav::render_core::DescriptorStorage;
using triglav::render_core::PipelineCache;
using triglav::render_core::RenderPassScope;
using triglav::render_core::ResourceStorage;
using triglav::test::TestingSupport;
using namespace triglav::name_literals;

namespace rt = triglav::graphics_api::ray_tracing;
namespace gapi = triglav::graphics_api;

namespace {

constexpr triglav::Vector2i DefaultSize{800, 600};

void execute_build_context(BuildContext& build_context, ResourceStorage& storage)
{
   PipelineCache pipeline_cache(TestingSupport::device(), TestingSupport::resource_manager());

   const auto job = build_context.build_job(pipeline_cache, storage);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   const gapi::SemaphoreArray empty_list;
   job.execute(0, empty_list, empty_list, &fence);

   fence.await();
}

[[nodiscard]] bool compare_stream_with_buffer(triglav::io::IReader& reader, const triglav::u8* buff_data,
                                              const triglav::MemorySize buff_size)
{
   static constexpr triglav::MemorySize CHUNK_SIZE{1024};

   std::array<triglav::u8, CHUNK_SIZE> chunk{};
   triglav::MemorySize buffer_offset = 0;

   while (true) {
      const auto res = reader.read(chunk);
      if (!res.has_value()) {
         return false;
      }

      const auto cmp_size = std::min(buff_size - buffer_offset, *res);

      if (std::memcmp(chunk.data(), buff_data + buffer_offset, cmp_size) != 0) {
         return false;
      }

      if (cmp_size != CHUNK_SIZE) {
         return true;
      }

      buffer_offset += cmp_size;
   }
}

triglav::u8 abs_diff(const triglav::u8 x, const triglav::u8 y)
{
   if (x > y) {
      return x - y;
   }
   return y - x;
}

[[nodiscard]] bool compare_stream_with_buffer_with_tolerance(triglav::io::IReader& reader, const triglav::u8* buff_data,
                                                             const triglav::MemorySize buff_size, const triglav::u8 tolerance)
{
   static constexpr triglav::MemorySize CHUNK_SIZE{1024};

   std::array<triglav::u8, CHUNK_SIZE> chunk{};
   triglav::MemorySize buffer_offset = 0;

   while (true) {
      const auto res = reader.read(chunk);
      if (!res.has_value()) {
         return false;
      }

      const auto cmp_size = std::min(buff_size - buffer_offset, *res);

      for (const auto i : triglav::Range(0u, cmp_size)) {
         if (abs_diff(chunk[i], buff_data[buffer_offset + i]) > tolerance) {
            return false;
         }
      }

      if (cmp_size != CHUNK_SIZE) {
         return true;
      }

      buffer_offset += cmp_size;
   }
}

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

[[maybe_unused]]
void dump_buffer(const std::span<const triglav::u8> buffer, const std::string_view file_name = "test_output.dat")
{
   const auto out_file = triglav::io::open_file(triglav::io::Path{file_name}, triglav::io::FileOpenMode::Create);
   assert(out_file.has_value());
   const auto write_res = (*out_file)->write(buffer);
   assert(write_res.has_value());
}

}// namespace

TEST(BuildContext, BasicCompute)
{
   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   // Declare resources
   build_context.declare_texture("test.basic_compute.pattern_texture"_name, {64, 64}, GAPI_FORMAT(RGBA, UNorm8));
   build_context.declare_staging_buffer("test.basic_compute.output_buffer"_name, 64 * 64 * sizeof(int));

   // Run compute
   build_context.bind_compute_shader("testing/basic_compute.cshader"_rc);
   build_context.bind_rw_texture(0, "test.basic_compute.pattern_texture"_name);
   build_context.dispatch({4, 4, 1});

   // Copy texture to output buffer
   build_context.copy_texture_to_buffer("test.basic_compute.pattern_texture"_name, "test.basic_compute.output_buffer"_name);

   // Run commands
   ResourceStorage storage(TestingSupport::device());
   execute_build_context(build_context, storage);

   // Verify pixel values
   auto& out_buffer = storage.buffer("test.basic_compute.output_buffer"_name, 0);
   const auto mapped_memory = GAPI_CHECK(out_buffer.map_memory());
   const auto* pixels = static_cast<triglav::u32*>(*mapped_memory);

   for (const auto y : triglav::Range(0, 64)) {
      for (const auto x : triglav::Range(0, 64)) {
         ASSERT_EQ(pixels[x + y * 64], (x + y) % 2 == 0 ? 0xFFFF00FF : 0xFF00FFFF);
      }
   }
}

TEST(BuildContext, BasicGraphics)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   static constexpr triglav::Vector2i dims{64, 64};
   static constexpr triglav::MemorySize buffer_size{sizeof(int) * dims.x * dims.y};

   // Declare resources
   build_context.declare_sized_render_target("test.basic_graphics.render_target"_name, dims, GAPI_FORMAT(RGBA, UNorm8));
   build_context.declare_staging_buffer("test.basic_graphics.output_buffer"_name, buffer_size);
   build_context.declare_buffer("test.basic_graphics.vertex_buffer"_name, 6 * sizeof(triglav::Vector4));
   build_context.declare_buffer("test.basic_graphics.uniform_buffer"_name, sizeof(triglav::Vector4));

   // Fill buffers with values
   build_context.fill_buffer("test.basic_graphics.vertex_buffer"_name, std::array{
                                                                          triglav::Vector4{-0.5f, -0.5f, 0.0f, 1.0f},
                                                                          triglav::Vector4{-0.5f, 0.5f, 0.0f, 1.0f},
                                                                          triglav::Vector4{0.5f, 0.5f, 0.0f, 1.0f},
                                                                          triglav::Vector4{-0.5f, -0.5f, 0.0f, 1.0f},
                                                                          triglav::Vector4{0.5f, 0.5f, 0.0f, 1.0f},
                                                                          triglav::Vector4{0.5f, -0.5f, 0.0f, 1.0f},
                                                                       });

   build_context.fill_buffer("test.basic_graphics.uniform_buffer"_name, triglav::Vector4{0, 0, 1, 1});

   {
      // Define render pass
      RenderPassScope scope(build_context, "test.basic_graphics.render_pass"_name, "test.basic_graphics.render_target"_name);

      // Define vertex shader, layout and input buffer
      build_context.bind_vertex_shader("testing/basic_graphics.vshader"_rc);

      triglav::render_core::VertexLayout layout(sizeof(triglav::Vector4));
      layout.add("position"_name, GAPI_FORMAT(RGBA, Float32), 0);
      build_context.bind_vertex_layout(layout);

      build_context.bind_vertex_buffer("test.basic_graphics.vertex_buffer"_name);

      // Bind fragment shader and ubo
      build_context.bind_fragment_shader("testing/basic_graphics.fshader"_rc);

      build_context.bind_uniform_buffer(0, "test.basic_graphics.uniform_buffer"_name);

      // Draw
      build_context.draw_primitives(6, 0);
   }

   // Copy the render target to staging buffer
   build_context.copy_texture_to_buffer("test.basic_graphics.render_target"_name, "test.basic_graphics.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(build_context, storage);

   auto& out_buffer = storage.buffer("test.basic_graphics.output_buffer"_name, 0);
   const auto mapped_memory = GAPI_CHECK(out_buffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mapped_memory);

   const auto expected_bitmap = open_file(Path{"content/basic_graphics_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expected_bitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expected_bitmap, pixels, buffer_size));
}

TEST(BuildContext, BasicDepth)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   static constexpr triglav::Vector2i dims{256, 256};
   static constexpr triglav::MemorySize buffer_size{sizeof(int) * dims.x * dims.y};

   auto box_mesh = triglav::geometry::create_box({2.0f, 2.0f, 2.0f});
   box_mesh.triangulate();
   const auto box_mesh_data = box_mesh.to_vertex_data();

   const auto view = glm::lookAt(triglav::Vector3{2.0f, 2.0f, 2.0f}, triglav::Vector3{0, 0, 0}, triglav::Vector3{0, 0, -1.0f});
   const auto perspective = glm::perspective(0.5f * static_cast<float>(triglav::geometry::g_pi), 1.0f, 0.1f, 100.0f);
   const auto transform = perspective * view;
   const auto offset = glm::translate(transform, triglav::Vector3{-2.0f, -2.0f, 1.3f});

   // Declare resources
   build_context.declare_sized_render_target("test.basic_depth.render_target"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   build_context.declare_sized_depth_target("test.basic_depth.depth_target"_name, dims, GAPI_FORMAT(D, UNorm16));
   build_context.declare_staging_buffer("test.basic_depth.output_buffer"_name, buffer_size);
   build_context.declare_buffer("test.basic_depth.vertex_buffer"_name, box_mesh_data.vertices.size() * sizeof(triglav::geometry::Vertex));
   build_context.declare_buffer("test.basic_depth.index_buffer"_name, box_mesh_data.indices.size() * sizeof(triglav::u32));
   build_context.declare_buffer("test.basic_depth.uniform_buffer"_name, 2 * sizeof(triglav::Matrix4x4));

   // Fill buffers with values
   build_context.fill_buffer_raw("test.basic_depth.vertex_buffer"_name, box_mesh_data.vertices.data(),
                                 box_mesh_data.vertices.size() * sizeof(triglav::geometry::Vertex));
   build_context.fill_buffer_raw("test.basic_depth.index_buffer"_name, box_mesh_data.indices.data(),
                                 box_mesh_data.indices.size() * sizeof(triglav::u32));
   build_context.fill_buffer("test.basic_depth.uniform_buffer"_name, std::array{transform, offset});

   {
      // Define render pass
      RenderPassScope scope(build_context, "test.basic_depth.render_pass"_name, "test.basic_depth.render_target"_name,
                            "test.basic_depth.depth_target"_name);

      // Define vertex shader, layout and input buffer
      build_context.bind_vertex_shader("testing/basic_depth.vshader"_rc);

      build_context.bind_uniform_buffer(0, "test.basic_depth.uniform_buffer"_name);

      triglav::render_core::VertexLayout layout(sizeof(triglav::geometry::Vertex));
      layout.add("location"_name, GAPI_FORMAT(RGB, Float32), offsetof(triglav::geometry::Vertex, location));
      build_context.bind_vertex_layout(layout);

      build_context.bind_vertex_buffer("test.basic_depth.vertex_buffer"_name);
      build_context.bind_index_buffer("test.basic_depth.index_buffer"_name);

      // Bind fragment shader and texture
      build_context.bind_fragment_shader("testing/basic_depth.fshader"_rc);

      // Draw
      build_context.draw_indexed_primitives(static_cast<u32>(box_mesh_data.indices.size()), 0, 0, 2, 0);
   }

   // Copy the render target to staging buffer
   build_context.copy_texture_to_buffer("test.basic_depth.render_target"_name, "test.basic_depth.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(build_context, storage);

   auto& out_buffer = storage.buffer("test.basic_depth.output_buffer"_name, 0);
   const auto mapped_memory = GAPI_CHECK(out_buffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mapped_memory);

   const auto expected_bitmap = open_file(Path{"content/basic_depth_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expected_bitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer_with_tolerance(**expected_bitmap, pixels, buffer_size, 0x02));
}

TEST(BuildContext, BasicTexture)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   struct Vertex
   {
      triglav::Vector2 position;
      triglav::Vector2 uv;
   };

   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   static constexpr triglav::Vector2i dims{128, 128};
   static constexpr triglav::MemorySize buffer_size{sizeof(int) * dims.x * dims.y};

   // Declare resources
   build_context.declare_sized_render_target("test.basic_texture.render_target"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   build_context.declare_staging_buffer("test.basic_texture.output_buffer"_name, buffer_size);
   build_context.declare_buffer("test.basic_texture.vertex_buffer"_name, 3 * sizeof(Vertex));

   static constexpr float sqrt_of_three = 1.73205f;
   static constexpr float y_offset = 1.0f - 0.5f * sqrt_of_three;

   // Fill buffers with values
   build_context.fill_buffer("test.basic_texture.vertex_buffer"_name, std::array{
                                                                         Vertex{{-1.0f, 1.0f - y_offset}, {0.0f, 1.0f}},
                                                                         Vertex{{1.0f, 1.0f - y_offset}, {1.0f, 1.0f}},
                                                                         Vertex{{0.0f, -1.0 + y_offset}, {0.5f, 0.0f}},
                                                                      });

   {
      // Define render pass
      RenderPassScope scope(build_context, "test.basic_texture.render_pass"_name, "test.basic_texture.render_target"_name);

      // Define vertex shader, layout and input buffer
      build_context.bind_vertex_shader("testing/basic_texture.vshader"_rc);

      triglav::render_core::VertexLayout layout(sizeof(Vertex));
      layout.add("position"_name, GAPI_FORMAT(RG, Float32), offsetof(Vertex, position));
      layout.add("uv"_name, GAPI_FORMAT(RG, Float32), offsetof(Vertex, uv));
      build_context.bind_vertex_layout(layout);

      build_context.bind_vertex_buffer("test.basic_texture.vertex_buffer"_name);

      // Bind fragment shader and texture
      build_context.bind_fragment_shader("testing/basic_texture.fshader"_rc);
      build_context.bind_samplable_texture(0, "testing/sample.tex"_rc);

      // Draw
      build_context.draw_primitives(3, 0);
   }

   // Copy the render target to staging buffer
   build_context.copy_texture_to_buffer("test.basic_texture.render_target"_name, "test.basic_texture.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(build_context, storage);

   auto& out_buffer = storage.buffer("test.basic_texture.output_buffer"_name, 0);
   const auto mapped_memory = GAPI_CHECK(out_buffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mapped_memory);

   const auto expected_bitmap = open_file(Path{"content/basic_texture_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expected_bitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expected_bitmap, pixels, buffer_size));
}

TEST(BuildContext, MultiplePasses)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   static constexpr triglav::Vector2i dims{128, 128};
   static constexpr triglav::MemorySize buffer_size{sizeof(int) * dims.x * dims.y};

   // Declare resources
   build_context.declare_sized_render_target("test.multiple_passes.render_target.first"_name, dims, GAPI_FORMAT(RGBA, UNorm8));
   build_context.declare_sized_render_target("test.multiple_passes.render_target.second"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   build_context.declare_staging_buffer("test.multiple_passes.output_buffer"_name, buffer_size);

   {
      // First Pass
      RenderPassScope scope(build_context, "test.multiple_passes.render_pass.first"_name, "test.multiple_passes.render_target.first"_name);
      build_context.bind_fragment_shader("testing/multiple_passes_first.fshader"_rc);
      build_context.draw_full_screen_quad();
   }
   {
      // Second pass
      RenderPassScope scope(build_context, "test.multiple_passes.render_pass.second"_name,
                            "test.multiple_passes.render_target.second"_name);
      build_context.bind_fragment_shader("testing/multiple_passes_second.fshader"_rc);
      build_context.bind_samplable_texture(0, "test.multiple_passes.render_target.first"_name);
      build_context.draw_full_screen_quad();
   }

   // Copy the render target to staging buffer
   build_context.copy_texture_to_buffer("test.multiple_passes.render_target.second"_name, "test.multiple_passes.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(build_context, storage);

   auto& out_buffer = storage.buffer("test.multiple_passes.output_buffer"_name, 0);
   const auto mapped_memory = GAPI_CHECK(out_buffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mapped_memory);

   const auto expected_bitmap = open_file(Path{"content/multiple_passes_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expected_bitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expected_bitmap, pixels, buffer_size));
}

TEST(BuildContext, DepthTargetSample)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   auto box_mesh = triglav::geometry::create_box({2.0f, 2.0f, 2.0f});
   box_mesh.triangulate();
   const auto box_mesh_data = box_mesh.to_vertex_data();

   const auto view = glm::lookAt(triglav::Vector3{2.0f, 2.0f, 2.0f}, triglav::Vector3{0, 0, 0}, triglav::Vector3{0, 0, -1.0f});
   const auto perspective = glm::perspective(0.5f * static_cast<float>(triglav::geometry::g_pi), 1.0f, 0.1f, 100.0f);
   const auto transform = perspective * view;

   static constexpr triglav::Vector2i dims{256, 256};
   static constexpr triglav::MemorySize buffer_size{sizeof(int) * dims.x * dims.y};

   // Declare resources
   build_context.declare_sized_depth_target("test.depth_target_sample.depth_target"_name, dims, GAPI_FORMAT(D, UNorm16));
   build_context.declare_sized_render_target("test.depth_target_sample.render_target"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   build_context.declare_staging_buffer("test.depth_target_sample.output_buffer"_name, buffer_size);
   build_context.declare_buffer("test.depth_target_sample.vertex_buffer"_name,
                                box_mesh_data.vertices.size() * sizeof(triglav::geometry::Vertex));
   build_context.declare_buffer("test.depth_target_sample.index_buffer"_name, box_mesh_data.indices.size() * sizeof(triglav::u32));
   build_context.declare_buffer("test.depth_target_sample.uniform_buffer"_name, sizeof(triglav::Matrix4x4));

   build_context.fill_buffer_raw("test.depth_target_sample.vertex_buffer"_name, box_mesh_data.vertices.data(),
                                 box_mesh_data.vertices.size() * sizeof(triglav::geometry::Vertex));
   build_context.fill_buffer_raw("test.depth_target_sample.index_buffer"_name, box_mesh_data.indices.data(),
                                 box_mesh_data.indices.size() * sizeof(triglav::u32));
   build_context.fill_buffer("test.depth_target_sample.uniform_buffer"_name, transform);

   {
      // Define render pass
      RenderPassScope scope(build_context, "test.depth_target_sample.depth_pass"_name, "test.depth_target_sample.depth_target"_name);

      // Define vertex shader, layout and input buffer
      build_context.bind_vertex_shader("testing/depth_target_sample/draw.vshader"_rc);

      build_context.bind_uniform_buffer(0, "test.depth_target_sample.uniform_buffer"_name);

      triglav::render_core::VertexLayout layout(sizeof(triglav::geometry::Vertex));
      layout.add("location"_name, GAPI_FORMAT(RGB, Float32), offsetof(triglav::geometry::Vertex, location));
      build_context.bind_vertex_layout(layout);

      build_context.bind_vertex_buffer("test.depth_target_sample.vertex_buffer"_name);
      build_context.bind_index_buffer("test.depth_target_sample.index_buffer"_name);

      // Bind fragment shader and texture
      build_context.bind_fragment_shader("testing/depth_target_sample/draw.fshader"_rc);

      // Draw
      build_context.draw_indexed_primitives(static_cast<u32>(box_mesh_data.indices.size()), 0, 0, 1, 0);
   }
   {
      // Second pass
      RenderPassScope scope(build_context, "test.depth_target_sample.render_pass"_name, "test.depth_target_sample.render_target"_name);
      build_context.bind_fragment_shader("testing/depth_target_sample/sample.fshader"_rc);
      build_context.bind_samplable_texture(0, "test.depth_target_sample.depth_target"_name);
      build_context.draw_full_screen_quad();
   }

   // Copy the render target to staging buffer
   build_context.copy_texture_to_buffer("test.depth_target_sample.render_target"_name, "test.depth_target_sample.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(build_context, storage);

   auto& out_buffer = storage.buffer("test.depth_target_sample.output_buffer"_name, 0);
   const auto mapped_memory = GAPI_CHECK(out_buffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mapped_memory);

   const auto expected_bitmap = open_file(Path{"content/depth_target_sample_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expected_bitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer_with_tolerance(**expected_bitmap, pixels, buffer_size, 0x02));
}

TEST(BuildContext, Conditions)
{
   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   build_context.declare_flag("copy_from_alpha"_name);

   build_context.declare_buffer("test.conditions.alpha"_name, sizeof(int));
   build_context.declare_buffer("test.conditions.beta"_name, sizeof(int));
   build_context.declare_staging_buffer("test.conditions.dst"_name, sizeof(int));

   build_context.fill_buffer("test.conditions.alpha"_name, 7);
   build_context.fill_buffer("test.conditions.beta"_name, 13);

   build_context.if_enabled("copy_from_alpha"_name);
   build_context.copy_buffer("test.conditions.alpha"_name, "test.conditions.dst"_name);
   build_context.end_if();

   build_context.if_disabled("copy_from_alpha"_name);
   build_context.copy_buffer("test.conditions.beta"_name, "test.conditions.dst"_name);
   build_context.end_if();

   PipelineCache pipeline_cache(TestingSupport::device(), TestingSupport::resource_manager());

   ResourceStorage storage(TestingSupport::device());

   auto job = build_context.build_job(pipeline_cache, storage);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   job.enable_flag("copy_from_alpha"_name);

   const gapi::SemaphoreArray empty_list;
   job.execute(0, empty_list, empty_list, &fence);

   fence.await();

   ASSERT_EQ(read_buffer<int>(storage.buffer("test.conditions.dst"_name, 0)), 7);

   job.disable_flag("copy_from_alpha"_name);
   job.execute(1, empty_list, empty_list, &fence);

   fence.await();

   ASSERT_EQ(read_buffer<int>(storage.buffer("test.conditions.dst"_name, 1)), 13);
}

TEST(BuildContext, CopyFromLastFrame)
{
   using namespace triglav::render_core::literals;

   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   build_context.declare_flag("has_buffer_changed"_name);
   build_context.declare_flag("copy_to_user"_name);

   build_context.declare_staging_buffer("test.copy_from_last_frame.user_buffer"_name, sizeof(int));
   build_context.declare_buffer("test.copy_from_last_frame.data_buffer"_name, sizeof(int));

   // If changed copy from the used buffer
   build_context.if_enabled("has_buffer_changed"_name);
   build_context.copy_buffer("test.copy_from_last_frame.user_buffer"_name, "test.copy_from_last_frame.data_buffer"_name);
   build_context.end_if();

   // If not changed copy from the previous frame
   build_context.if_disabled("has_buffer_changed"_name);
   build_context.copy_buffer("test.copy_from_last_frame.data_buffer"_last_frame, "test.copy_from_last_frame.data_buffer"_name);
   build_context.end_if();

   build_context.bind_compute_shader("testing/increase_number.cshader"_rc);
   build_context.bind_storage_buffer(0, "test.copy_from_last_frame.data_buffer"_name);
   build_context.dispatch({1, 1, 1});

   build_context.if_enabled("copy_to_user"_name);
   build_context.copy_buffer("test.copy_from_last_frame.data_buffer"_name, "test.copy_from_last_frame.user_buffer"_name);
   build_context.end_if();

   PipelineCache pipeline_cache(TestingSupport::device(), TestingSupport::resource_manager());
   ResourceStorage storage(TestingSupport::device());
   auto job = build_context.build_job(pipeline_cache, storage);

   write_buffer(storage.buffer("test.copy_from_last_frame.user_buffer"_name, 0), 36);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   const gapi::SemaphoreArray empty_list;

   const auto first_sem = GAPI_CHECK(TestingSupport::device().create_semaphore());
   const auto second_sem = GAPI_CHECK(TestingSupport::device().create_semaphore());
   const auto third_sem = GAPI_CHECK(TestingSupport::device().create_semaphore());

   gapi::SemaphoreArray list_first;
   list_first.add_semaphore(first_sem);
   gapi::SemaphoreArray list_second;
   list_second.add_semaphore(second_sem);
   gapi::SemaphoreArray list_third;
   list_third.add_semaphore(third_sem);

   job.enable_flag("has_buffer_changed"_name);

   job.execute(0, empty_list, list_first, nullptr);

   job.disable_flag("has_buffer_changed"_name);

   for ([[maybe_unused]] const auto _ : triglav::Range(0, 10)) {
      job.execute(1, list_first, list_second, nullptr);
      job.execute(2, list_second, list_third, nullptr);
      job.execute(0, list_third, list_first, &fence);

      fence.await();
   }

   job.enable_flag("copy_to_user"_name);

   job.execute(1, list_first, empty_list, &fence);

   fence.await();

   ASSERT_EQ(read_buffer<int>(storage.buffer("test.copy_from_last_frame.user_buffer"_name, 1)), 36 + 2 + 10 * 3);
}

TEST(BuildContext, ConditionalBarrier)
{
   using namespace triglav::render_core::literals;

   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   build_context.declare_flag("increase_number"_name);
   build_context.init_buffer("test.conditional_barrier.data"_name, 5);

   build_context.if_enabled("increase_number"_name);
   build_context.bind_compute_shader("testing/increase_number.cshader"_rc);
   build_context.bind_storage_buffer(0, "test.conditional_barrier.data"_name);
   build_context.dispatch({1, 1, 1});
   build_context.end_if();

   build_context.declare_staging_buffer("test.conditional_barrier.user"_name, sizeof(int));
   build_context.copy_buffer("test.conditional_barrier.data"_name, "test.conditional_barrier.user"_name);

   PipelineCache pipeline_cache(TestingSupport::device(), TestingSupport::resource_manager());
   ResourceStorage storage(TestingSupport::device());
   auto job = build_context.build_job(pipeline_cache, storage);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   const gapi::SemaphoreArray empty_list;

   job.execute(0, empty_list, empty_list, &fence);
   fence.await();
   ASSERT_EQ(read_buffer<int>(storage.buffer("test.conditional_barrier.user"_name, 0)), 5);

   job.enable_flag("increase_number"_name);
   job.execute(0, empty_list, empty_list, &fence);
   fence.await();
   ASSERT_EQ(read_buffer<int>(storage.buffer("test.conditional_barrier.user"_name, 0)), 6);
}

#if TG_RAY_TRACING_TESTS

TEST(BuildContext, BasicRayTracing)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   gapi::BufferHeap as_buffer_heap(TestingSupport::device(), gapi::BufferUsage::AccelerationStructure | gapi::BufferUsage::StorageBuffer);
   rt::AccelerationStructurePool as_pool(TestingSupport::device());

   rt::GeometryBuildContext bottom_level_ctx(TestingSupport::device(), as_pool, as_buffer_heap);

   auto box_mesh = triglav::geometry::create_box({2.0f, 2.0f, 2.0f});
   box_mesh.triangulate();
   const auto box_mesh_data =
      box_mesh.upload_to_device(TestingSupport::device(), gapi::BufferUsage::TransferDst | gapi::BufferUsage::AccelerationStructureRead);

   bottom_level_ctx.add_triangle_buffer(box_mesh_data.mesh.vertices.buffer(), box_mesh_data.mesh.indices.buffer(),
                                        GAPI_FORMAT(RGB, Float32), sizeof(triglav::geometry::Vertex),
                                        static_cast<u32>(box_mesh_data.mesh.vertices.count()),
                                        static_cast<u32>(box_mesh_data.mesh.indices.count() / 3));

   auto* box_as = bottom_level_ctx.commit_triangles();

   auto build_blcmd_list = GAPI_CHECK(TestingSupport::device().create_command_list(gapi::WorkType::Compute));
   GAPI_CHECK_STATUS(build_blcmd_list.begin(gapi::SubmitType::OneTime));
   bottom_level_ctx.build_acceleration_structures(build_blcmd_list);
   GAPI_CHECK_STATUS(build_blcmd_list.finish());
   GAPI_CHECK_STATUS(TestingSupport::device().submit_command_list_one_time(build_blcmd_list));

   rt::GeometryBuildContext top_level_ctx(TestingSupport::device(), as_pool, as_buffer_heap);

   rt::InstanceBuilder instance_builder(TestingSupport::device());
   instance_builder.add_instance(*box_as, triglav::Matrix4x4(1), 0);
   auto instance_buffer = instance_builder.build_buffer();

   top_level_ctx.add_instance_buffer(instance_buffer, 1);

   auto* top_level_as = top_level_ctx.commit_instances();

   auto build_tlcmd_list = GAPI_CHECK(TestingSupport::device().create_command_list(gapi::WorkType::Compute));
   GAPI_CHECK_STATUS(build_tlcmd_list.begin(gapi::SubmitType::OneTime));
   top_level_ctx.build_acceleration_structures(build_tlcmd_list);
   GAPI_CHECK_STATUS(build_tlcmd_list.finish());
   GAPI_CHECK_STATUS(TestingSupport::device().submit_command_list_one_time(build_tlcmd_list));

   static constexpr triglav::MemorySize buffer_size{sizeof(int) * DefaultSize.x * DefaultSize.y};

   BuildContext build_context(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   build_context.declare_screen_size_texture("test.basic_ray_tracing.target"_name, GAPI_FORMAT(RGBA, UNorm8));
   build_context.declare_staging_buffer("test.basic_ray_tracing.output"_name, buffer_size);

   const auto view = glm::lookAt(triglav::Vector3{2.0f, 2.0f, 2.0f}, triglav::Vector3{0, 0, 0}, triglav::Vector3{0, 0, -1.0f});
   const auto perspective = glm::perspective(0.5f * static_cast<float>(triglav::g_pi),
                                             static_cast<float>(DefaultSize.x) / static_cast<float>(DefaultSize.y), 0.1f, 100.0f);

   struct ViewProperties
   {
      triglav::Matrix4x4 proj_inverse;
      triglav::Matrix4x4 view_inverse;
   };

   build_context.init_buffer("test.basic_ray_tracing.view_props"_name, ViewProperties{inverse(perspective), inverse(view)});

   build_context.bind_rt_generation_shader("testing/rt/basic_rgen.rgenshader"_rc);
   build_context.bind_acceleration_structure(0, *top_level_as);
   build_context.bind_rw_texture(1, "test.basic_ray_tracing.target"_name);
   build_context.bind_uniform_buffer(2, "test.basic_ray_tracing.view_props"_name);

   build_context.bind_rt_closest_hit_shader("testing/rt/basic_rchit.rchitshader"_rc);
   build_context.bind_rt_miss_shader("testing/rt/basic_rmiss.rmissshader"_rc);

   triglav::render_core::RayTracingShaderGroup rt_gen_group{};
   rt_gen_group.type = triglav::render_core::RayTracingShaderGroupType::General;
   rt_gen_group.general_shader.emplace("testing/rt/basic_rgen.rgenshader"_rc);
   build_context.bind_rt_shader_group(rt_gen_group);

   triglav::render_core::RayTracingShaderGroup rt_miss_group{};
   rt_miss_group.type = triglav::render_core::RayTracingShaderGroupType::General;
   rt_miss_group.general_shader.emplace("testing/rt/basic_rmiss.rmissshader"_rc);
   build_context.bind_rt_shader_group(rt_miss_group);

   triglav::render_core::RayTracingShaderGroup rt_closest_hit_group{};
   rt_closest_hit_group.type = triglav::render_core::RayTracingShaderGroupType::Triangles;
   rt_closest_hit_group.closest_hit_shader.emplace("testing/rt/basic_rchit.rchitshader"_rc);
   build_context.bind_rt_shader_group(rt_closest_hit_group);

   build_context.trace_rays({DefaultSize.x, DefaultSize.y, 1});

   build_context.copy_texture_to_buffer("test.basic_ray_tracing.target"_name, "test.basic_ray_tracing.output"_name);

   PipelineCache pipeline_cache(TestingSupport::device(), TestingSupport::resource_manager());

   ResourceStorage storage(TestingSupport::device());
   auto job = build_context.build_job(pipeline_cache, storage);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   const gapi::SemaphoreArray empty_list;
   job.execute(0, empty_list, empty_list, &fence);

   fence.await();

   auto& out_buffer = storage.buffer("test.basic_ray_tracing.output"_name, 0);
   const auto mapped_memory = GAPI_CHECK(out_buffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mapped_memory);

   const auto expected_bitmap = open_file(Path{"content/basic_ray_tracing_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expected_bitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer_with_tolerance(**expected_bitmap, pixels, buffer_size, 0x02));
}

#endif
