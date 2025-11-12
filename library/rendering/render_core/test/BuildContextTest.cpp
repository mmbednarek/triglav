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

void execute_build_context(BuildContext& buildContext, ResourceStorage& storage)
{
   PipelineCache pipelineCache(TestingSupport::device(), TestingSupport::resource_manager());

   const auto job = buildContext.build_job(pipelineCache, storage);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   const gapi::SemaphoreArray emptyList;
   job.execute(0, emptyList, emptyList, &fence);

   fence.await();
}

[[nodiscard]] bool compare_stream_with_buffer(triglav::io::IReader& reader, const triglav::u8* buffData, const triglav::MemorySize buffSize)
{
   static constexpr triglav::MemorySize CHUNK_SIZE{1024};

   std::array<triglav::u8, CHUNK_SIZE> chunk{};
   triglav::MemorySize bufferOffset = 0;

   while (true) {
      const auto res = reader.read(chunk);
      if (!res.has_value()) {
         return false;
      }

      const auto cmpSize = std::min(buffSize - bufferOffset, *res);

      if (std::memcmp(chunk.data(), buffData + bufferOffset, cmpSize) != 0) {
         return false;
      }

      if (cmpSize != CHUNK_SIZE) {
         return true;
      }

      bufferOffset += cmpSize;
   }
}

triglav::u8 abs_diff(const triglav::u8 x, const triglav::u8 y)
{
   if (x > y) {
      return x - y;
   }
   return y - x;
}

[[nodiscard]] bool compare_stream_with_buffer_with_tolerance(triglav::io::IReader& reader, const triglav::u8* buffData,
                                                             const triglav::MemorySize buffSize, const triglav::u8 tolerance)
{
   static constexpr triglav::MemorySize CHUNK_SIZE{1024};

   std::array<triglav::u8, CHUNK_SIZE> chunk{};
   triglav::MemorySize bufferOffset = 0;

   while (true) {
      const auto res = reader.read(chunk);
      if (!res.has_value()) {
         return false;
      }

      const auto cmpSize = std::min(buffSize - bufferOffset, *res);

      for (const auto i : triglav::Range(0u, cmpSize)) {
         if (abs_diff(chunk[i], buffData[bufferOffset + i]) > tolerance) {
            return false;
         }
      }

      if (cmpSize != CHUNK_SIZE) {
         return true;
      }

      bufferOffset += cmpSize;
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
void dump_buffer(const std::span<const triglav::u8> buffer, const std::string_view fileName = "test_output.dat")
{
   const auto outFile = triglav::io::open_file(triglav::io::Path{fileName}, triglav::io::FileOpenMode::Create);
   assert(outFile.has_value());
   const auto write_res = (*outFile)->write(buffer);
   assert(write_res.has_value());
}

}// namespace

TEST(BuildContext, BasicCompute)
{
   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   // Declare resources
   buildContext.declare_texture("test.basic_compute.pattern_texture"_name, {64, 64}, GAPI_FORMAT(RGBA, UNorm8));
   buildContext.declare_staging_buffer("test.basic_compute.output_buffer"_name, 64 * 64 * sizeof(int));

   // Run compute
   buildContext.bind_compute_shader("testing/basic_compute.cshader"_rc);
   buildContext.bind_rw_texture(0, "test.basic_compute.pattern_texture"_name);
   buildContext.dispatch({4, 4, 1});

   // Copy texture to output buffer
   buildContext.copy_texture_to_buffer("test.basic_compute.pattern_texture"_name, "test.basic_compute.output_buffer"_name);

   // Run commands
   ResourceStorage storage(TestingSupport::device());
   execute_build_context(buildContext, storage);

   // Verify pixel values
   auto& outBuffer = storage.buffer("test.basic_compute.output_buffer"_name, 0);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u32*>(*mappedMemory);

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

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   static constexpr triglav::Vector2i dims{64, 64};
   static constexpr triglav::MemorySize bufferSize{sizeof(int) * dims.x * dims.y};

   // Declare resources
   buildContext.declare_sized_render_target("test.basic_graphics.render_target"_name, dims, GAPI_FORMAT(RGBA, UNorm8));
   buildContext.declare_staging_buffer("test.basic_graphics.output_buffer"_name, bufferSize);
   buildContext.declare_buffer("test.basic_graphics.vertex_buffer"_name, 6 * sizeof(triglav::Vector4));
   buildContext.declare_buffer("test.basic_graphics.uniform_buffer"_name, sizeof(triglav::Vector4));

   // Fill buffers with values
   buildContext.fill_buffer("test.basic_graphics.vertex_buffer"_name, std::array{
                                                                         triglav::Vector4{-0.5f, -0.5f, 0.0f, 1.0f},
                                                                         triglav::Vector4{-0.5f, 0.5f, 0.0f, 1.0f},
                                                                         triglav::Vector4{0.5f, 0.5f, 0.0f, 1.0f},
                                                                         triglav::Vector4{-0.5f, -0.5f, 0.0f, 1.0f},
                                                                         triglav::Vector4{0.5f, 0.5f, 0.0f, 1.0f},
                                                                         triglav::Vector4{0.5f, -0.5f, 0.0f, 1.0f},
                                                                      });

   buildContext.fill_buffer("test.basic_graphics.uniform_buffer"_name, triglav::Vector4{0, 0, 1, 1});

   {
      // Define render pass
      RenderPassScope scope(buildContext, "test.basic_graphics.render_pass"_name, "test.basic_graphics.render_target"_name);

      // Define vertex shader, layout and input buffer
      buildContext.bind_vertex_shader("testing/basic_graphics.vshader"_rc);

      triglav::render_core::VertexLayout layout(sizeof(triglav::Vector4));
      layout.add("position"_name, GAPI_FORMAT(RGBA, Float32), 0);
      buildContext.bind_vertex_layout(layout);

      buildContext.bind_vertex_buffer("test.basic_graphics.vertex_buffer"_name);

      // Bind fragment shader and ubo
      buildContext.bind_fragment_shader("testing/basic_graphics.fshader"_rc);

      buildContext.bind_uniform_buffer(0, "test.basic_graphics.uniform_buffer"_name);

      // Draw
      buildContext.draw_primitives(6, 0);
   }

   // Copy the render target to staging buffer
   buildContext.copy_texture_to_buffer("test.basic_graphics.render_target"_name, "test.basic_graphics.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage.buffer("test.basic_graphics.output_buffer"_name, 0);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap = open_file(Path{"content/basic_graphics_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expectedBitmap, pixels, bufferSize));
}

TEST(BuildContext, BasicDepth)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   static constexpr triglav::Vector2i dims{256, 256};
   static constexpr triglav::MemorySize bufferSize{sizeof(int) * dims.x * dims.y};

   auto boxMesh = triglav::geometry::create_box({2.0f, 2.0f, 2.0f});
   boxMesh.triangulate();
   const auto boxMeshData = boxMesh.to_vertex_data();

   const auto view = glm::lookAt(triglav::Vector3{2.0f, 2.0f, 2.0f}, triglav::Vector3{0, 0, 0}, triglav::Vector3{0, 0, -1.0f});
   const auto perspective = glm::perspective(0.5f * static_cast<float>(triglav::geometry::g_pi), 1.0f, 0.1f, 100.0f);
   const auto transform = perspective * view;
   const auto offset = glm::translate(transform, triglav::Vector3{-2.0f, -2.0f, 1.3f});

   // Declare resources
   buildContext.declare_sized_render_target("test.basic_depth.render_target"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   buildContext.declare_sized_depth_target("test.basic_depth.depth_target"_name, dims, GAPI_FORMAT(D, UNorm16));
   buildContext.declare_staging_buffer("test.basic_depth.output_buffer"_name, bufferSize);
   buildContext.declare_buffer("test.basic_depth.vertex_buffer"_name, boxMeshData.vertices.size() * sizeof(triglav::geometry::Vertex));
   buildContext.declare_buffer("test.basic_depth.index_buffer"_name, boxMeshData.indices.size() * sizeof(triglav::u32));
   buildContext.declare_buffer("test.basic_depth.uniform_buffer"_name, 2 * sizeof(triglav::Matrix4x4));

   // Fill buffers with values
   buildContext.fill_buffer_raw("test.basic_depth.vertex_buffer"_name, boxMeshData.vertices.data(),
                                boxMeshData.vertices.size() * sizeof(triglav::geometry::Vertex));
   buildContext.fill_buffer_raw("test.basic_depth.index_buffer"_name, boxMeshData.indices.data(),
                                boxMeshData.indices.size() * sizeof(triglav::u32));
   buildContext.fill_buffer("test.basic_depth.uniform_buffer"_name, std::array{transform, offset});

   {
      // Define render pass
      RenderPassScope scope(buildContext, "test.basic_depth.render_pass"_name, "test.basic_depth.render_target"_name,
                            "test.basic_depth.depth_target"_name);

      // Define vertex shader, layout and input buffer
      buildContext.bind_vertex_shader("testing/basic_depth.vshader"_rc);

      buildContext.bind_uniform_buffer(0, "test.basic_depth.uniform_buffer"_name);

      triglav::render_core::VertexLayout layout(sizeof(triglav::geometry::Vertex));
      layout.add("location"_name, GAPI_FORMAT(RGB, Float32), offsetof(triglav::geometry::Vertex, location));
      buildContext.bind_vertex_layout(layout);

      buildContext.bind_vertex_buffer("test.basic_depth.vertex_buffer"_name);
      buildContext.bind_index_buffer("test.basic_depth.index_buffer"_name);

      // Bind fragment shader and texture
      buildContext.bind_fragment_shader("testing/basic_depth.fshader"_rc);

      // Draw
      buildContext.draw_indexed_primitives(static_cast<u32>(boxMeshData.indices.size()), 0, 0, 2, 0);
   }

   // Copy the render target to staging buffer
   buildContext.copy_texture_to_buffer("test.basic_depth.render_target"_name, "test.basic_depth.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage.buffer("test.basic_depth.output_buffer"_name, 0);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap = open_file(Path{"content/basic_depth_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer_with_tolerance(**expectedBitmap, pixels, bufferSize, 0x02));
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

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   static constexpr triglav::Vector2i dims{128, 128};
   static constexpr triglav::MemorySize bufferSize{sizeof(int) * dims.x * dims.y};

   // Declare resources
   buildContext.declare_sized_render_target("test.basic_texture.render_target"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   buildContext.declare_staging_buffer("test.basic_texture.output_buffer"_name, bufferSize);
   buildContext.declare_buffer("test.basic_texture.vertex_buffer"_name, 3 * sizeof(Vertex));

   static constexpr float sqrtOfThree = 1.73205f;
   static constexpr float yOffset = 1.0f - 0.5f * sqrtOfThree;

   // Fill buffers with values
   buildContext.fill_buffer("test.basic_texture.vertex_buffer"_name, std::array{
                                                                        Vertex{{-1.0f, 1.0f - yOffset}, {0.0f, 1.0f}},
                                                                        Vertex{{1.0f, 1.0f - yOffset}, {1.0f, 1.0f}},
                                                                        Vertex{{0.0f, -1.0 + yOffset}, {0.5f, 0.0f}},
                                                                     });

   {
      // Define render pass
      RenderPassScope scope(buildContext, "test.basic_texture.render_pass"_name, "test.basic_texture.render_target"_name);

      // Define vertex shader, layout and input buffer
      buildContext.bind_vertex_shader("testing/basic_texture.vshader"_rc);

      triglav::render_core::VertexLayout layout(sizeof(Vertex));
      layout.add("position"_name, GAPI_FORMAT(RG, Float32), offsetof(Vertex, position));
      layout.add("uv"_name, GAPI_FORMAT(RG, Float32), offsetof(Vertex, uv));
      buildContext.bind_vertex_layout(layout);

      buildContext.bind_vertex_buffer("test.basic_texture.vertex_buffer"_name);

      // Bind fragment shader and texture
      buildContext.bind_fragment_shader("testing/basic_texture.fshader"_rc);
      buildContext.bind_samplable_texture(0, "testing/sample.tex"_rc);

      // Draw
      buildContext.draw_primitives(3, 0);
   }

   // Copy the render target to staging buffer
   buildContext.copy_texture_to_buffer("test.basic_texture.render_target"_name, "test.basic_texture.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage.buffer("test.basic_texture.output_buffer"_name, 0);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap = open_file(Path{"content/basic_texture_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expectedBitmap, pixels, bufferSize));
}

TEST(BuildContext, MultiplePasses)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   static constexpr triglav::Vector2i dims{128, 128};
   static constexpr triglav::MemorySize bufferSize{sizeof(int) * dims.x * dims.y};

   // Declare resources
   buildContext.declare_sized_render_target("test.multiple_passes.render_target.first"_name, dims, GAPI_FORMAT(RGBA, UNorm8));
   buildContext.declare_sized_render_target("test.multiple_passes.render_target.second"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   buildContext.declare_staging_buffer("test.multiple_passes.output_buffer"_name, bufferSize);

   {
      // First Pass
      RenderPassScope scope(buildContext, "test.multiple_passes.render_pass.first"_name, "test.multiple_passes.render_target.first"_name);
      buildContext.bind_fragment_shader("testing/multiple_passes_first.fshader"_rc);
      buildContext.draw_full_screen_quad();
   }
   {
      // Second pass
      RenderPassScope scope(buildContext, "test.multiple_passes.render_pass.second"_name, "test.multiple_passes.render_target.second"_name);
      buildContext.bind_fragment_shader("testing/multiple_passes_second.fshader"_rc);
      buildContext.bind_samplable_texture(0, "test.multiple_passes.render_target.first"_name);
      buildContext.draw_full_screen_quad();
   }

   // Copy the render target to staging buffer
   buildContext.copy_texture_to_buffer("test.multiple_passes.render_target.second"_name, "test.multiple_passes.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage.buffer("test.multiple_passes.output_buffer"_name, 0);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap = open_file(Path{"content/multiple_passes_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expectedBitmap, pixels, bufferSize));
}

TEST(BuildContext, DepthTargetSample)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   auto boxMesh = triglav::geometry::create_box({2.0f, 2.0f, 2.0f});
   boxMesh.triangulate();
   const auto boxMeshData = boxMesh.to_vertex_data();

   const auto view = glm::lookAt(triglav::Vector3{2.0f, 2.0f, 2.0f}, triglav::Vector3{0, 0, 0}, triglav::Vector3{0, 0, -1.0f});
   const auto perspective = glm::perspective(0.5f * static_cast<float>(triglav::geometry::g_pi), 1.0f, 0.1f, 100.0f);
   const auto transform = perspective * view;

   static constexpr triglav::Vector2i dims{256, 256};
   static constexpr triglav::MemorySize bufferSize{sizeof(int) * dims.x * dims.y};

   // Declare resources
   buildContext.declare_sized_depth_target("test.depth_target_sample.depth_target"_name, dims, GAPI_FORMAT(D, UNorm16));
   buildContext.declare_sized_render_target("test.depth_target_sample.render_target"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   buildContext.declare_staging_buffer("test.depth_target_sample.output_buffer"_name, bufferSize);
   buildContext.declare_buffer("test.depth_target_sample.vertex_buffer"_name,
                               boxMeshData.vertices.size() * sizeof(triglav::geometry::Vertex));
   buildContext.declare_buffer("test.depth_target_sample.index_buffer"_name, boxMeshData.indices.size() * sizeof(triglav::u32));
   buildContext.declare_buffer("test.depth_target_sample.uniform_buffer"_name, sizeof(triglav::Matrix4x4));

   buildContext.fill_buffer_raw("test.depth_target_sample.vertex_buffer"_name, boxMeshData.vertices.data(),
                                boxMeshData.vertices.size() * sizeof(triglav::geometry::Vertex));
   buildContext.fill_buffer_raw("test.depth_target_sample.index_buffer"_name, boxMeshData.indices.data(),
                                boxMeshData.indices.size() * sizeof(triglav::u32));
   buildContext.fill_buffer("test.depth_target_sample.uniform_buffer"_name, transform);

   {
      // Define render pass
      RenderPassScope scope(buildContext, "test.depth_target_sample.depth_pass"_name, "test.depth_target_sample.depth_target"_name);

      // Define vertex shader, layout and input buffer
      buildContext.bind_vertex_shader("testing/depth_target_sample/draw.vshader"_rc);

      buildContext.bind_uniform_buffer(0, "test.depth_target_sample.uniform_buffer"_name);

      triglav::render_core::VertexLayout layout(sizeof(triglav::geometry::Vertex));
      layout.add("location"_name, GAPI_FORMAT(RGB, Float32), offsetof(triglav::geometry::Vertex, location));
      buildContext.bind_vertex_layout(layout);

      buildContext.bind_vertex_buffer("test.depth_target_sample.vertex_buffer"_name);
      buildContext.bind_index_buffer("test.depth_target_sample.index_buffer"_name);

      // Bind fragment shader and texture
      buildContext.bind_fragment_shader("testing/depth_target_sample/draw.fshader"_rc);

      // Draw
      buildContext.draw_indexed_primitives(static_cast<u32>(boxMeshData.indices.size()), 0, 0, 1, 0);
   }
   {
      // Second pass
      RenderPassScope scope(buildContext, "test.depth_target_sample.render_pass"_name, "test.depth_target_sample.render_target"_name);
      buildContext.bind_fragment_shader("testing/depth_target_sample/sample.fshader"_rc);
      buildContext.bind_samplable_texture(0, "test.depth_target_sample.depth_target"_name);
      buildContext.draw_full_screen_quad();
   }

   // Copy the render target to staging buffer
   buildContext.copy_texture_to_buffer("test.depth_target_sample.render_target"_name, "test.depth_target_sample.output_buffer"_name);

   ResourceStorage storage(TestingSupport::device());
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage.buffer("test.depth_target_sample.output_buffer"_name, 0);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap = open_file(Path{"content/depth_target_sample_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer_with_tolerance(**expectedBitmap, pixels, bufferSize, 0x02));
}

TEST(BuildContext, Conditions)
{
   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   buildContext.declare_flag("copy_from_alpha"_name);

   buildContext.declare_buffer("test.conditions.alpha"_name, sizeof(int));
   buildContext.declare_buffer("test.conditions.beta"_name, sizeof(int));
   buildContext.declare_staging_buffer("test.conditions.dst"_name, sizeof(int));

   buildContext.fill_buffer("test.conditions.alpha"_name, 7);
   buildContext.fill_buffer("test.conditions.beta"_name, 13);

   buildContext.if_enabled("copy_from_alpha"_name);
   buildContext.copy_buffer("test.conditions.alpha"_name, "test.conditions.dst"_name);
   buildContext.end_if();

   buildContext.if_disabled("copy_from_alpha"_name);
   buildContext.copy_buffer("test.conditions.beta"_name, "test.conditions.dst"_name);
   buildContext.end_if();

   PipelineCache pipelineCache(TestingSupport::device(), TestingSupport::resource_manager());

   ResourceStorage storage(TestingSupport::device());

   auto job = buildContext.build_job(pipelineCache, storage);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   job.enable_flag("copy_from_alpha"_name);

   const gapi::SemaphoreArray emptyList;
   job.execute(0, emptyList, emptyList, &fence);

   fence.await();

   ASSERT_EQ(read_buffer<int>(storage.buffer("test.conditions.dst"_name, 0)), 7);

   job.disable_flag("copy_from_alpha"_name);
   job.execute(1, emptyList, emptyList, &fence);

   fence.await();

   ASSERT_EQ(read_buffer<int>(storage.buffer("test.conditions.dst"_name, 1)), 13);
}

TEST(BuildContext, CopyFromLastFrame)
{
   using namespace triglav::render_core::literals;

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   buildContext.declare_flag("has_buffer_changed"_name);
   buildContext.declare_flag("copy_to_user"_name);

   buildContext.declare_staging_buffer("test.copy_from_last_frame.user_buffer"_name, sizeof(int));
   buildContext.declare_buffer("test.copy_from_last_frame.data_buffer"_name, sizeof(int));

   // If changed copy from the used buffer
   buildContext.if_enabled("has_buffer_changed"_name);
   buildContext.copy_buffer("test.copy_from_last_frame.user_buffer"_name, "test.copy_from_last_frame.data_buffer"_name);
   buildContext.end_if();

   // If not changed copy from the previous frame
   buildContext.if_disabled("has_buffer_changed"_name);
   buildContext.copy_buffer("test.copy_from_last_frame.data_buffer"_last_frame, "test.copy_from_last_frame.data_buffer"_name);
   buildContext.end_if();

   buildContext.bind_compute_shader("testing/increase_number.cshader"_rc);
   buildContext.bind_storage_buffer(0, "test.copy_from_last_frame.data_buffer"_name);
   buildContext.dispatch({1, 1, 1});

   buildContext.if_enabled("copy_to_user"_name);
   buildContext.copy_buffer("test.copy_from_last_frame.data_buffer"_name, "test.copy_from_last_frame.user_buffer"_name);
   buildContext.end_if();

   PipelineCache pipelineCache(TestingSupport::device(), TestingSupport::resource_manager());
   ResourceStorage storage(TestingSupport::device());
   auto job = buildContext.build_job(pipelineCache, storage);

   write_buffer(storage.buffer("test.copy_from_last_frame.user_buffer"_name, 0), 36);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   const gapi::SemaphoreArray emptyList;

   const auto firstSem = GAPI_CHECK(TestingSupport::device().create_semaphore());
   const auto secondSem = GAPI_CHECK(TestingSupport::device().create_semaphore());
   const auto thirdSem = GAPI_CHECK(TestingSupport::device().create_semaphore());

   gapi::SemaphoreArray listFirst;
   listFirst.add_semaphore(firstSem);
   gapi::SemaphoreArray listSecond;
   listSecond.add_semaphore(secondSem);
   gapi::SemaphoreArray listThird;
   listThird.add_semaphore(thirdSem);

   job.enable_flag("has_buffer_changed"_name);

   job.execute(0, emptyList, listFirst, nullptr);

   job.disable_flag("has_buffer_changed"_name);

   for ([[maybe_unused]] const auto _ : triglav::Range(0, 10)) {
      job.execute(1, listFirst, listSecond, nullptr);
      job.execute(2, listSecond, listThird, nullptr);
      job.execute(0, listThird, listFirst, &fence);

      fence.await();
   }

   job.enable_flag("copy_to_user"_name);

   job.execute(1, listFirst, emptyList, &fence);

   fence.await();

   ASSERT_EQ(read_buffer<int>(storage.buffer("test.copy_from_last_frame.user_buffer"_name, 1)), 36 + 2 + 10 * 3);
}

TEST(BuildContext, ConditionalBarrier)
{
   using namespace triglav::render_core::literals;

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   buildContext.declare_flag("increase_number"_name);
   buildContext.init_buffer("test.conditional_barrier.data"_name, 5);

   buildContext.if_enabled("increase_number"_name);
   buildContext.bind_compute_shader("testing/increase_number.cshader"_rc);
   buildContext.bind_storage_buffer(0, "test.conditional_barrier.data"_name);
   buildContext.dispatch({1, 1, 1});
   buildContext.end_if();

   buildContext.declare_staging_buffer("test.conditional_barrier.user"_name, sizeof(int));
   buildContext.copy_buffer("test.conditional_barrier.data"_name, "test.conditional_barrier.user"_name);

   PipelineCache pipelineCache(TestingSupport::device(), TestingSupport::resource_manager());
   ResourceStorage storage(TestingSupport::device());
   auto job = buildContext.build_job(pipelineCache, storage);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   const gapi::SemaphoreArray emptyList;

   job.execute(0, emptyList, emptyList, &fence);
   fence.await();
   ASSERT_EQ(read_buffer<int>(storage.buffer("test.conditional_barrier.user"_name, 0)), 5);

   job.enable_flag("increase_number"_name);
   job.execute(0, emptyList, emptyList, &fence);
   fence.await();
   ASSERT_EQ(read_buffer<int>(storage.buffer("test.conditional_barrier.user"_name, 0)), 6);
}

#if TG_RAY_TRACING_TESTS

TEST(BuildContext, BasicRayTracing)
{
   using triglav::io::FileOpenMode;
   using triglav::io::open_file;
   using triglav::io::Path;

   gapi::BufferHeap asBufferHeap(TestingSupport::device(), gapi::BufferUsage::AccelerationStructure | gapi::BufferUsage::StorageBuffer);
   rt::AccelerationStructurePool asPool(TestingSupport::device());

   rt::GeometryBuildContext bottomLevelCtx(TestingSupport::device(), asPool, asBufferHeap);

   auto boxMesh = triglav::geometry::create_box({2.0f, 2.0f, 2.0f});
   boxMesh.triangulate();
   const auto boxMeshData =
      boxMesh.upload_to_device(TestingSupport::device(), gapi::BufferUsage::TransferDst | gapi::BufferUsage::AccelerationStructureRead);

   bottomLevelCtx.add_triangle_buffer(boxMeshData.mesh.vertices.buffer(), boxMeshData.mesh.indices.buffer(), GAPI_FORMAT(RGB, Float32),
                                      sizeof(triglav::geometry::Vertex), static_cast<u32>(boxMeshData.mesh.vertices.count()),
                                      static_cast<u32>(boxMeshData.mesh.indices.count() / 3));

   auto* boxAS = bottomLevelCtx.commit_triangles();

   auto buildBLCmdList = GAPI_CHECK(TestingSupport::device().create_command_list(gapi::WorkType::Compute));
   GAPI_CHECK_STATUS(buildBLCmdList.begin(gapi::SubmitType::OneTime));
   bottomLevelCtx.build_acceleration_structures(buildBLCmdList);
   GAPI_CHECK_STATUS(buildBLCmdList.finish());
   GAPI_CHECK_STATUS(TestingSupport::device().submit_command_list_one_time(buildBLCmdList));

   rt::GeometryBuildContext topLevelCtx(TestingSupport::device(), asPool, asBufferHeap);

   rt::InstanceBuilder instanceBuilder(TestingSupport::device());
   instanceBuilder.add_instance(*boxAS, triglav::Matrix4x4(1), 0);
   auto instanceBuffer = instanceBuilder.build_buffer();

   topLevelCtx.add_instance_buffer(instanceBuffer, 1);

   auto* topLevelAS = topLevelCtx.commit_instances();

   auto buildTLCmdList = GAPI_CHECK(TestingSupport::device().create_command_list(gapi::WorkType::Compute));
   GAPI_CHECK_STATUS(buildTLCmdList.begin(gapi::SubmitType::OneTime));
   topLevelCtx.build_acceleration_structures(buildTLCmdList);
   GAPI_CHECK_STATUS(buildTLCmdList.finish());
   GAPI_CHECK_STATUS(TestingSupport::device().submit_command_list_one_time(buildTLCmdList));

   static constexpr triglav::MemorySize bufferSize{sizeof(int) * DefaultSize.x * DefaultSize.y};

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager(), DefaultSize);

   buildContext.declare_screen_size_texture("test.basic_ray_tracing.target"_name, GAPI_FORMAT(RGBA, UNorm8));
   buildContext.declare_staging_buffer("test.basic_ray_tracing.output"_name, bufferSize);

   const auto view = glm::lookAt(triglav::Vector3{2.0f, 2.0f, 2.0f}, triglav::Vector3{0, 0, 0}, triglav::Vector3{0, 0, -1.0f});
   const auto perspective = glm::perspective(0.5f * static_cast<float>(triglav::g_pi),
                                             static_cast<float>(DefaultSize.x) / static_cast<float>(DefaultSize.y), 0.1f, 100.0f);

   struct ViewProperties
   {
      triglav::Matrix4x4 projInverse;
      triglav::Matrix4x4 viewInverse;
   };

   buildContext.init_buffer("test.basic_ray_tracing.view_props"_name, ViewProperties{inverse(perspective), inverse(view)});

   buildContext.bind_rt_generation_shader("testing/rt/basic_rgen.rgenshader"_rc);
   buildContext.bind_acceleration_structure(0, *topLevelAS);
   buildContext.bind_rw_texture(1, "test.basic_ray_tracing.target"_name);
   buildContext.bind_uniform_buffer(2, "test.basic_ray_tracing.view_props"_name);

   buildContext.bind_rt_closest_hit_shader("testing/rt/basic_rchit.rchitshader"_rc);
   buildContext.bind_rt_miss_shader("testing/rt/basic_rmiss.rmissshader"_rc);

   triglav::render_core::RayTracingShaderGroup rtGenGroup{};
   rtGenGroup.type = triglav::render_core::RayTracingShaderGroupType::General;
   rtGenGroup.generalShader.emplace("testing/rt/basic_rgen.rgenshader"_rc);
   buildContext.bind_rt_shader_group(rtGenGroup);

   triglav::render_core::RayTracingShaderGroup rtMissGroup{};
   rtMissGroup.type = triglav::render_core::RayTracingShaderGroupType::General;
   rtMissGroup.generalShader.emplace("testing/rt/basic_rmiss.rmissshader"_rc);
   buildContext.bind_rt_shader_group(rtMissGroup);

   triglav::render_core::RayTracingShaderGroup rtClosestHitGroup{};
   rtClosestHitGroup.type = triglav::render_core::RayTracingShaderGroupType::Triangles;
   rtClosestHitGroup.closestHitShader.emplace("testing/rt/basic_rchit.rchitshader"_rc);
   buildContext.bind_rt_shader_group(rtClosestHitGroup);

   buildContext.trace_rays({DefaultSize.x, DefaultSize.y, 1});

   buildContext.copy_texture_to_buffer("test.basic_ray_tracing.target"_name, "test.basic_ray_tracing.output"_name);

   PipelineCache pipelineCache(TestingSupport::device(), TestingSupport::resource_manager());

   ResourceStorage storage(TestingSupport::device());
   auto job = buildContext.build_job(pipelineCache, storage);

   const auto fence = GAPI_CHECK(TestingSupport::device().create_fence());
   fence.await();

   const gapi::SemaphoreArray emptyList;
   job.execute(0, emptyList, emptyList, &fence);

   fence.await();

   auto& outBuffer = storage.buffer("test.basic_ray_tracing.output"_name, 0);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap = open_file(Path{"content/basic_ray_tracing_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer_with_tolerance(**expectedBitmap, pixels, bufferSize, 0x02));
}

#endif
