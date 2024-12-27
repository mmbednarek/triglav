#include "TestingSupport.hpp"

#include "triglav/Math.hpp"
#include "triglav/Ranges.hpp"
#include "triglav/geometry/DebugMesh.hpp"
#include "triglav/graphics_api/DescriptorLayoutCache.hpp"
#include "triglav/io/File.hpp"
#include "triglav/render_core/BuildContext.hpp"

#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <gtest/gtest.h>
#include <renderdoc_app.h>

using triglav::render_core::BufferRef;
using triglav::render_core::BuildContext;
using triglav::render_core::DescriptorStorage;
using triglav::render_core::PipelineCache;
using triglav::render_core::RenderPassScope;
using triglav::render_core::ResourceStorage;
using triglav::test::TestingSupport;
using namespace triglav::name_literals;

namespace gapi = triglav::graphics_api;

namespace {

void execute_build_context(BuildContext& buildContext, const std::span<ResourceStorage> storage)
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

[[maybe_unused]]
void dump_buffer(const std::span<const triglav::u8> buffer)
{
   const auto outFile = triglav::io::open_file(triglav::io::Path{"test_output.dat"}, triglav::io::FileOpenMode::Create);
   assert(outFile.has_value());
   assert((*outFile)->write(buffer).has_value());
}

}// namespace

TEST(BuildContext, BasicCompute)
{
   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager());

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
   std::array<ResourceStorage, triglav::render_core::FRAMES_IN_FLIGHT_COUNT> storage;
   execute_build_context(buildContext, storage);

   // Verify pixel values
   auto& outBuffer = storage[0].buffer("test.basic_compute.output_buffer"_name);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<int*>(*mappedMemory);

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

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager());

   static constexpr triglav::Vector2i dims{64, 64};
   static constexpr triglav::MemorySize bufferSize{sizeof(int) * dims.x * dims.y};

   // Declare resources
   buildContext.declare_render_target_with_dims("test.basic_graphics.render_target"_name, dims, GAPI_FORMAT(RGBA, UNorm8));
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

   std::array<ResourceStorage, triglav::render_core::FRAMES_IN_FLIGHT_COUNT> storage;
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage[0].buffer("test.basic_graphics.output_buffer"_name);
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

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager());

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
   buildContext.declare_render_target_with_dims("test.basic_depth.render_target"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   buildContext.declare_depth_target_with_dims("test.basic_depth.depth_target"_name, dims, GAPI_FORMAT(D, UNorm16));
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

      triglav::render_core::VertexLayout layout(sizeof(triglav::geometry::Vertex));
      layout.add("location"_name, GAPI_FORMAT(RGB, Float32), offsetof(triglav::geometry::Vertex, location));
      buildContext.bind_vertex_layout(layout);

      buildContext.bind_vertex_buffer("test.basic_depth.vertex_buffer"_name);
      buildContext.bind_index_buffer("test.basic_depth.index_buffer"_name);
      buildContext.bind_uniform_buffer(0, "test.basic_depth.uniform_buffer"_name);

      // Bind fragment shader and texture
      buildContext.bind_fragment_shader("testing/basic_depth.fshader"_rc);

      // Draw
      buildContext.draw_indexed_primitives(boxMeshData.indices.size(), 0, 0, 2, 0);
   }

   // Copy the render target to staging buffer
   buildContext.copy_texture_to_buffer("test.basic_depth.render_target"_name, "test.basic_depth.output_buffer"_name);

   std::array<ResourceStorage, triglav::render_core::FRAMES_IN_FLIGHT_COUNT> storage;
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage[0].buffer("test.basic_depth.output_buffer"_name);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap = open_file(Path{"content/basic_depth_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expectedBitmap, pixels, bufferSize));
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

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager());

   static constexpr triglav::Vector2i dims{128, 128};
   static constexpr triglav::MemorySize bufferSize{sizeof(int) * dims.x * dims.y};

   // Declare resources
   buildContext.declare_render_target_with_dims("test.basic_texture.render_target"_name, dims, GAPI_FORMAT(RGBA, sRGB));
   buildContext.declare_staging_buffer("test.basic_texture.output_buffer"_name, bufferSize);
   buildContext.declare_buffer("test.basic_texture.vertex_buffer"_name, 3 * sizeof(Vertex));

   static constexpr float sqrtOfThree = std::sqrt(3.0f);
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

   std::array<ResourceStorage, triglav::render_core::FRAMES_IN_FLIGHT_COUNT> storage;
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage[0].buffer("test.basic_texture.output_buffer"_name);
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

   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager());

   static constexpr triglav::Vector2i dims{128, 128};
   static constexpr triglav::MemorySize bufferSize{sizeof(int) * dims.x * dims.y};

   // Declare resources
   buildContext.declare_render_target_with_dims("test.multiple_passes.render_target.first"_name, dims, GAPI_FORMAT(RGBA, UNorm8));
   buildContext.declare_render_target_with_dims("test.multiple_passes.render_target.second"_name, dims, GAPI_FORMAT(RGBA, sRGB));
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

   std::array<ResourceStorage, triglav::render_core::FRAMES_IN_FLIGHT_COUNT> storage;
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage[0].buffer("test.multiple_passes.output_buffer"_name);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap = open_file(Path{"content/multiple_passes_expected_bitmap.dat"}, FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expectedBitmap, pixels, bufferSize));
}
