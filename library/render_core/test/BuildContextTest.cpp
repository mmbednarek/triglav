#include "TestingSupport.hpp"

#include "triglav/Math.hpp"
#include "triglav/Ranges.hpp"
#include "triglav/graphics_api/DescriptorLayoutCache.hpp"
#include "triglav/io/File.hpp"
#include "triglav/render_core/BuildContext.hpp"

#include <cstring>
#include <gtest/gtest.h>

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

}// namespace

TEST(BuildContext, BasicCompute)
{
   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager());

   // Declare resources
   buildContext.declare_texture("test.pattern_texture"_name, {64, 64}, GAPI_FORMAT(RGBA, UNorm8));
   buildContext.declare_staging_buffer("test.output_buffer"_name, 64 * 64 * sizeof(int));

   // Run compute
   buildContext.bind_compute_shader("testing/basic_compute.cshader"_rc);
   buildContext.bind_rw_texture(0, "test.pattern_texture"_name);
   buildContext.dispatch({4, 4, 1});

   // Copy texture to output buffer
   buildContext.copy_texture_to_buffer("test.pattern_texture"_name, "test.output_buffer"_name);

   // Run commands
   std::array<ResourceStorage, triglav::render_core::FRAMES_IN_FLIGHT_COUNT> storage;
   execute_build_context(buildContext, storage);

   // Verify pixel values
   auto& outBuffer = storage[0].buffer("test.output_buffer"_name);
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
   BuildContext buildContext(TestingSupport::device(), TestingSupport::resource_manager());

   // Declare resources
   buildContext.declare_render_target_with_dims("test.render_target"_name, {64, 64}, GAPI_FORMAT(RGBA, UNorm8));
   buildContext.declare_staging_buffer("test.output_buffer"_name, 64 * 64 * sizeof(int));
   buildContext.declare_buffer("test.vertex_buffer"_name, 6 * sizeof(triglav::Vector4));
   buildContext.declare_buffer("test.uniform_buffer"_name, sizeof(triglav::Vector4));

   // Fill buffers with values
   buildContext.fill_buffer("test.vertex_buffer"_name, std::array{
                                                          triglav::Vector4{-0.5f, -0.5f, 0.0f, 1.0f},
                                                          triglav::Vector4{-0.5f, 0.5f, 0.0f, 1.0f},
                                                          triglav::Vector4{0.5f, 0.5f, 0.0f, 1.0f},
                                                          triglav::Vector4{-0.5f, -0.5f, 0.0f, 1.0f},
                                                          triglav::Vector4{0.5f, 0.5f, 0.0f, 1.0f},
                                                          triglav::Vector4{0.5f, -0.5f, 0.0f, 1.0f},
                                                       });

   buildContext.fill_buffer("test.uniform_buffer"_name, triglav::Vector4{0, 0, 1, 1});

   {
      // Define render pass
      RenderPassScope scope(buildContext, "test.render_pass"_name, "test.render_target"_name);

      // Define vertex shader, layout and input buffer
      buildContext.bind_vertex_shader("testing/basic_graphics.vshader"_rc);

      triglav::render_core::VertexLayout layout(sizeof(triglav::Vector4));
      layout.add("position"_name, GAPI_FORMAT(RGBA, Float32), 0);
      buildContext.bind_vertex_layout(layout);

      buildContext.bind_vertex_buffer("test.vertex_buffer"_name);

      // Bind fragment shader and ubo
      buildContext.bind_fragment_shader("testing/basic_graphics.fshader"_rc);

      buildContext.bind_uniform_buffer(0, "test.uniform_buffer"_name);

      // Draw
      buildContext.draw_primitives(6, 0);
   }

   // Copy the render target to staging buffer
   buildContext.copy_texture_to_buffer("test.render_target"_name, "test.output_buffer"_name);

   std::array<ResourceStorage, triglav::render_core::FRAMES_IN_FLIGHT_COUNT> storage;
   execute_build_context(buildContext, storage);

   auto& outBuffer = storage[0].buffer("test.output_buffer"_name);
   const auto mappedMemory = GAPI_CHECK(outBuffer.map_memory());
   const auto* pixels = static_cast<triglav::u8*>(*mappedMemory);

   const auto expectedBitmap =
      triglav::io::open_file(triglav::io::Path{"content/basic_graphics_expected_bitmap.dat"}, triglav::io::FileOpenMode::Read);
   ASSERT_TRUE(expectedBitmap.has_value());

   ASSERT_TRUE(compare_stream_with_buffer(**expectedBitmap, pixels, 64 * 64 * sizeof(int)));
}
