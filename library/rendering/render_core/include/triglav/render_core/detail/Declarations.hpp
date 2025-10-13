#pragma once

#include "../RenderCore.hpp"

namespace triglav::render_core::detail {

namespace decl {

struct Texture
{
   Name texName{};
   std::optional<Vector2i> texDims{};// none means screen-size
   graphics_api::ColorFormat texFormat{};
   graphics_api::TextureUsageFlags texUsageFlags{};
   bool createMipLevels{false};
   std::optional<float> scaling{};
   std::array<graphics_api::TextureState, 24> currentStatePerMip;
   std::array<graphics_api::PipelineStageFlags, 24> lastStages;
   TextureBarrier* lastTextureBarrier{};
   graphics_api::SamplerProperties samplerProperties{
      graphics_api::FilterType::Linear,
      graphics_api::FilterType::Linear,
      graphics_api::TextureAddressMode::Repeat,
      graphics_api::TextureAddressMode::Repeat,
      graphics_api::TextureAddressMode::Repeat,
      true,
      0.0f,
      0.0f,
      0.0f,
   };

   [[nodiscard]] Vector2i dimensions(const Vector2i& screenDim) const
   {
      auto size = this->texDims.value_or(screenDim);
      if (this->scaling.has_value()) {
         size = Vector2i(Vector2(size) * this->scaling.value());
      }
      return size;
   }
};

struct Buffer
{
   Name buffName{};
   MemorySize buffSize{};
   graphics_api::BufferUsageFlags buffUsageFlags{};
   std::optional<float> scale{};
   graphics_api::PipelineStageFlags lastStages{};
   graphics_api::BufferAccess currentAccess{graphics_api::BufferAccess::None};
   BufferBarrier* lastBufferBarrier{};
};

}// namespace decl

using Declaration = std::variant<decl::Texture, decl::Buffer>;

}// namespace triglav::render_core::detail
