#include "RenderCore.hpp"

#include "triglav/Ranges.hpp"

namespace triglav::render_core {

namespace {

constexpr std::array<PipelineHash, 16> DESCRIPTOR_PRIMES = {
   69885709, 97554461, 31478333, 89779681, 56906909, 20587033, 32938993, 67627577,
   98405159, 64800523, 22023377, 59743291, 56747209, 93652771, 40912363, 36661543,
};

constexpr std::array<PipelineHash, 16> VERTEX_ATTRIBUTE_PRIMES = {
   46919, 37781, 29819, 34849, 21611, 33547, 87133, 93083, 44131, 62921, 29587, 13109, 28607, 46307, 72871, 11699,
};

constexpr std::array<PipelineHash, 16> RENDER_TARGET_PRIMES = {
   575591, 266909, 477149, 320759, 138323, 180811, 280411, 759637, 620869, 364303, 291377, 229037, 202931, 226663, 901529, 862777,
};

constexpr std::array<PipelineHash, 16> PUSH_CONSTANT_PRIMES = {
   5660561, 6982051, 2132797, 9986183, 2804227, 6939677, 9608461, 8126431,
   2447399, 3563953, 2897371, 6209569, 8146571, 1354267, 9918409, 1721689,
};

constexpr std::array<PipelineHash, 16> RAY_CLOSEST_HIT_PRIMES = {
   28437797, 20944817, 91184309, 22773103, 53008649, 83805661, 25533763, 27557219,
   84381529, 65679151, 40842881, 15755609, 87173981, 54957989, 29610533, 53679599,
};

constexpr std::array<PipelineHash, 16> RAY_MISS_PRIMES = {
   60386867, 14597347, 88038959, 20446493, 74318963, 63180391, 70454897, 25012079,
   58513783, 60817397, 43172621, 91417979, 59783069, 26571007, 95041489, 49736153,
};

constexpr std::array<PipelineHash, 16> RAY_SHADER_GROUP_PRIMES = {
   11959853, 31284833, 84292073, 46503707, 82198463, 22559489, 44030387, 58136597,
   42053233, 99472067, 58265849, 11079947, 32325577, 64498283, 88087333, 86433041,
};

PipelineHash hash_color_format(const graphics_api::ColorFormat& format)
{
   return 76037261 * static_cast<PipelineHash>(format.order) + 25809323 * static_cast<PipelineHash>(format.parts[0]) +
          73029629 * static_cast<PipelineHash>(format.parts[1]) + 15383051 * static_cast<PipelineHash>(format.parts[2]) +
          21467893 * static_cast<PipelineHash>(format.parts[3]);
}

}// namespace

PipelineHash DescriptorInfo::hash() const
{
   return 193043 * static_cast<u32>(this->descriptor_type) + 225349 * static_cast<u32>(this->pipeline_stages.value) +
          4411217 * this->descriptor_count;
}

PipelineHash DescriptorState::hash() const
{
   PipelineHash result{};

   for (const auto i : Range(0u, this->descriptor_count)) {
      result += DESCRIPTOR_PRIMES[i] * this->descriptors[i].hash();
   }

   return result;
}

PipelineHash VertexAttribute::hash() const
{
   return 89883559 * name + 16934917 * hash_color_format(format) + 72575779 * offset;
}

VertexLayout::VertexLayout(const u32 stride) :
    stride(stride)
{
}

VertexLayout& VertexLayout::add(const Name name, const graphics_api::ColorFormat& format, const u32 offset)
{
   attributes.emplace_back(name, format, offset);
   return *this;
}

PipelineHash VertexLayout::hash() const
{
   PipelineHash result{};
   result += 16885391 * stride;
   for (const auto [index, attribute] : Enumerate(attributes)) {
      result += VERTEX_ATTRIBUTE_PRIMES[index] * attribute.hash();
   }
   return result;
}

PipelineHash PushConstantDesc::hash() const
{
   return 72142307 * this->flags.value + 28790857 * this->size;
}

PipelineHash GraphicPipelineState::hash() const
{
   PipelineHash result{};

   if (this->fragment_shader.has_value()) {
      result += 3791173 * this->fragment_shader->name();
   }
   if (this->vertex_shader.has_value()) {
      result += 1252177 * this->vertex_shader->name();
   }
   result += 1887451 * this->vertex_layout.hash();
   result += 5973041 * this->descriptor_state.hash();

   for (const auto [index, format] : Enumerate(render_target_formats)) {
      result += RENDER_TARGET_PRIMES[index] * hash_color_format(format);
   }
   if (depth_target_format.has_value()) {
      result += 23116759 * hash_color_format(*depth_target_format);
   }
   result += 4276381 * static_cast<u32>(vertex_topology);
   result += 2723521 * static_cast<u32>(depth_test_mode);
   result += 6378731 * static_cast<u32>(is_blending_enabled);
   result += 61075103 * static_cast<u32>(line_width * 10.0f);

   for (const auto [index, push_constant] : Enumerate(this->push_constants)) {
      result += PUSH_CONSTANT_PRIMES[index] * push_constant.hash();
   }

   return result;
}

PipelineHash ComputePipelineState::hash() const
{
   PipelineHash result{};

   if (this->compute_shader.has_value()) {
      result += 221955337 * this->compute_shader.value().name();
   }

   result += 733777621 * this->descriptor_state.hash();

   return result;
}

PipelineHash RayTracingShaderGroup::hash() const
{
   PipelineHash result = 625888457 * static_cast<u32>(type);
   if (general_shader.has_value()) {
      result += 469726181 * general_shader->name();
   }
   if (closest_hit_shader.has_value()) {
      result += 290082467 * closest_hit_shader->name();
   }
   return result;
}

void RayTracingPipelineState::reset()
{
   this->descriptor_state.descriptor_count = 0;
   this->max_recursion = 4;
   this->push_constants.clear();
   this->shader_groups.clear();
   this->ray_gen_shader.reset();
   this->ray_miss_shaders.clear();
   this->ray_closest_hit_shaders.clear();
}

std::vector<Name> RayTracingPipelineState::shader_bindings() const
{
   std::vector<Name> result;
   if (this->ray_gen_shader.has_value()) {
      result.emplace_back(make_rt_shader_name(*this->ray_gen_shader));
   }
   for (const auto shader : this->ray_miss_shaders) {
      result.emplace_back(make_rt_shader_name(shader));
   }
   for (const auto shader : this->ray_closest_hit_shaders) {
      result.emplace_back(make_rt_shader_name(shader));
   }
   return result;
}

PipelineHash RayTracingPipelineState::hash() const
{
   PipelineHash result{};

   if (ray_gen_shader.has_value()) {
      result += 46410383 * ray_gen_shader->name();
   }
   for (const auto [index, shader] : Enumerate(ray_closest_hit_shaders)) {
      result += RAY_CLOSEST_HIT_PRIMES[index] * ray_gen_shader->name();
   }
   for (const auto [index, shader] : Enumerate(ray_miss_shaders)) {
      result += RAY_MISS_PRIMES[index] * ray_gen_shader->name();
   }
   result += 612708209 * descriptor_state.hash();
   for (const auto [index, push_constant] : Enumerate(push_constants)) {
      result += PUSH_CONSTANT_PRIMES[index] * push_constant.hash();
   }
   result += 795496733 * max_recursion;
   for (const auto [index, shader_group] : Enumerate(shader_groups)) {
      result += RAY_SHADER_GROUP_PRIMES[index] * shader_group.hash();
   }

   return result;
}

}// namespace triglav::render_core
