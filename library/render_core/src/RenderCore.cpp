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

PipelineHash hash_color_format(const graphics_api::ColorFormat& format)
{
   return 76037261 * static_cast<PipelineHash>(format.order) + 25809323 * static_cast<PipelineHash>(format.parts[0]) +
          73029629 * static_cast<PipelineHash>(format.parts[1]) + 15383051 * static_cast<PipelineHash>(format.parts[2]) +
          21467893 * static_cast<PipelineHash>(format.parts[3]);
}

}// namespace

PipelineHash DescriptorInfo::hash() const
{
   return 193043 * static_cast<u32>(this->descriptorType) + 225349 * static_cast<u32>(this->pipelineStages.value) +
          4411217 * this->descriptorCount;
}

PipelineHash DescriptorState::hash() const
{
   PipelineHash result{};

   for (const auto i : Range(0u, this->descriptorCount)) {
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

void VertexLayout::add(const Name name, const graphics_api::ColorFormat& format, const u32 offset)
{
   attributes.emplace_back(name, format, offset);
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

PipelineHash GraphicPipelineState::hash() const
{
   PipelineHash result{};

   if (this->fragmentShader.has_value()) {
      result += 3791173 * this->fragmentShader->name();
   }
   if (this->vertexShader.has_value()) {
      result += 1252177 * this->vertexShader->name();
   }
   result += 1887451 * this->vertexLayout.hash();
   result += 5973041 * this->descriptorState.hash();

   for (const auto [index, format] : Enumerate(renderTargetFormats)) {
      result += RENDER_TARGET_PRIMES[index] * hash_color_format(format);
   }
   if (depthTargetFormat.has_value()) {
      result += 23116759 * hash_color_format(*depthTargetFormat);
   }
   result += 4276381 * static_cast<u32>(vertexTopology);

   return result;
}

PipelineHash ComputePipelineState::hash() const
{
   PipelineHash result{};

   if (this->computeShader.has_value()) {
      result += 221955337 * this->computeShader.value().name();
   }

   result += 733777621 * this->descriptorState.hash();

   return result;
}

}// namespace triglav::render_core
