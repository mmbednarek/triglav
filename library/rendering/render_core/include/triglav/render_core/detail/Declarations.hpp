#pragma once

#include "../RenderCore.hpp"

namespace triglav::render_core::detail {

namespace decl {

struct Texture
{
   Name tex_name{};
   std::optional<Vector2i> tex_dims{};// none means screen-size
   graphics_api::ColorFormat tex_format{};
   graphics_api::TextureUsageFlags tex_usage_flags{};
   bool create_mip_levels{false};
   std::optional<float> scaling{};
   std::array<graphics_api::TextureState, 24> current_state_per_mip;
   std::array<graphics_api::PipelineStageFlags, 24> last_stages;
   TextureBarrier* last_texture_barrier{};
   graphics_api::SamplerProperties sampler_properties{
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

   [[nodiscard]] Vector2i dimensions(const Vector2i& screen_dim) const
   {
      auto size = this->tex_dims.value_or(screen_dim);
      if (this->scaling.has_value()) {
         size = Vector2i(Vector2(size) * this->scaling.value());
      }
      return size;
   }
};

struct Buffer
{
   Name buff_name{};
   MemorySize buff_size{};
   graphics_api::BufferUsageFlags buff_usage_flags{};
   std::optional<float> scale{};
   graphics_api::PipelineStageFlags last_stages{};
   graphics_api::BufferAccess current_access{graphics_api::BufferAccess::None};
   BufferBarrier* last_buffer_barrier{};
};

}// namespace decl

using Declaration = std::variant<decl::Texture, decl::Buffer>;

}// namespace triglav::render_core::detail
