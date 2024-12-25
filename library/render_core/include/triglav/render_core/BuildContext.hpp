#pragma once

#include "Job.hpp"
#include "PipelineCache.hpp"
#include "RenderCore.hpp"
#include "ResourceStorage.hpp"

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/graphics_api/GraphicsApi.hpp"

#include <map>
#include <optional>
#include <variant>
#include <vector>

namespace triglav::graphics_api {
class CommandList;
class Pipeline;
class Device;
}// namespace triglav::graphics_api

namespace triglav::render_core {

namespace detail {

namespace descriptor {

struct RWTexture
{
   Name texName{};
};

struct UniformBuffer
{
   Name buffName{};
};

struct CombinedTexSampler
{
   TextureName texName;
};

}// namespace descriptor

using Descriptor = std::variant<descriptor::RWTexture, descriptor::CombinedTexSampler, descriptor::UniformBuffer>;

namespace decl {

struct Texture
{
   Name texName{};
   Vector2i texDims{};
   graphics_api::ColorFormat texFormat{};
   graphics_api::TextureUsageFlags texUsageFlags{};
   graphics_api::TextureState currentState{graphics_api::TextureState::Undefined};
   graphics_api::PipelineStage lastUsedStage{graphics_api::PipelineStage::Entrypoint};
};

struct RenderTarget
{
   Name rtName{};
   std::optional<Vector2i> rtDims;
   graphics_api::ColorFormat rtFormat{};
   graphics_api::ClearValue clearValue{};
   graphics_api::AttachmentAttributeFlags flags;
   bool isDepthTarget{false};
   graphics_api::TextureUsageFlags texUsageFlags{};
   graphics_api::TextureState currentState{graphics_api::TextureState::Undefined};
   graphics_api::PipelineStage lastUsedStage{graphics_api::PipelineStage::Entrypoint};
};

struct Buffer
{
   Name buffName{};
   MemorySize buffSize{};
   graphics_api::BufferUsageFlags buffUsageFlags{};
   graphics_api::PipelineStage lastUsedStage{graphics_api::PipelineStage::Entrypoint};
   bool isInTransferState{false};
};

}// namespace decl

using Declaration = std::variant<decl::Texture, decl::RenderTarget, decl::Buffer>;

namespace cmd {

struct BindGraphicsPipeline
{
   GraphicPipelineState pso;
};

struct BindComputePipeline
{
   ComputePipelineState pso;
};

struct BindDescriptors
{
   std::vector<std::optional<Descriptor>> descriptors{};
};

struct BindVertexBuffer
{
   Name buffName{};
};

struct TextureTransition
{
   Name texName{};
   graphics_api::PipelineStageFlags srcStageFlags{};
   graphics_api::TextureState srcState{};
   graphics_api::PipelineStageFlags dstStageFlags{};
   graphics_api::TextureState dstState{};
};

struct ExecutionBarrier
{
   graphics_api::PipelineStageFlags srcStageFlags{};
   graphics_api::PipelineStageFlags dstStageFlags{};
};

struct DrawPrimitives
{
   u32 vertexCount{};
   u32 vertexOffset{};
   u32 instanceCount{};
   u32 instanceOffset{};
};

struct Dispatch
{
   Vector3i dims;
};

struct CopyTextureToBuffer
{
   Name srcTexture;
   Name dstBuffer;
};

struct FillBuffer
{
   Name buffName{};
   std::vector<u8> data{};
};

struct BeginRenderPass
{
   Name passName;
   std::vector<Name> renderTargets;
};

struct EndRenderPass
{};

}// namespace cmd

using Command = std::variant<cmd::BindGraphicsPipeline, cmd::BindComputePipeline, cmd::DrawPrimitives, cmd::Dispatch, cmd::BindDescriptors,
                             cmd::BindVertexBuffer, cmd::CopyTextureToBuffer, cmd::TextureTransition, cmd::ExecutionBarrier,
                             cmd::FillBuffer, cmd::BeginRenderPass, cmd::EndRenderPass>;

struct DescriptorCounts
{
   u32 storageImageCount{};
   u32 uniformBufferCount{};
   u32 totalDescriptorSets{};
};

}// namespace detail

class BuildContext
{
 public:
   BuildContext(graphics_api::Device& device, resource::ResourceManager& resourceManager);

   // Declarations
   void declare_texture(Name texName, Vector2i texDims, graphics_api::ColorFormat texFormat = GAPI_FORMAT(RGBA, sRGB));
   void declare_render_target(Name rtName, graphics_api::ColorFormat rtFormat = GAPI_FORMAT(RGBA, UNorm8));
   void declare_render_target_with_dims(Name rtName, Vector2i rtDims, graphics_api::ColorFormat rtFormat = GAPI_FORMAT(RGBA, UNorm8));
   void declare_buffer(Name buffName, MemorySize size);
   void declare_staging_buffer(Name buffName, MemorySize size);

   // Shader binding
   void bind_vertex_shader(VertexShaderName vsName);
   void bind_fragment_shader(FragmentShaderName fsName);
   void bind_compute_shader(ComputeShaderName csName);

   // Descriptor binding
   void bind_rw_texture(BindingIndex index, Name texName);
   void bind_uniform_buffer(BindingIndex index, Name buffName);

   // Vertex binding
   void bind_vertex_layout(const VertexLayout& layout);
   void bind_vertex_buffer(Name buffName);

   // Render pass
   void begin_render_pass_raw(Name passName, std::span<Name> renderTargets);
   void end_render_pass();

   template<typename... TArgs>
   void begin_render_pass(Name passName, TArgs... args)
   {
      std::array<Name, sizeof...(TArgs)> renderTargets{args...};
      this->begin_render_pass_raw(passName, renderTargets);
   }

   // Drawing
   void draw_primitives(u32 vertexCount, u32 vertexOffset);

   // Execution
   void dispatch(Vector3i dims);

   // Transfer
   void fill_buffer_raw(Name buffName, const void* ptr, MemorySize size);
   void copy_texture_to_buffer(Name srcTex, Name dstBuff);

   template<typename TData>
   void fill_buffer(const Name buffName, const TData& data)
   {
      this->fill_buffer_raw(buffName, reinterpret_cast<const void*>(&data), sizeof(data));
   }

   Job build_job(PipelineCache& pipelineCache, std::span<ResourceStorage> storages);

   void write_commands(ResourceStorage& storage, DescriptorStorage& descStorage, graphics_api::CommandList& cmdList, PipelineCache& cache,
                       graphics_api::DescriptorPool& pool);

   void create_resources(ResourceStorage& storage);
   [[nodiscard]] graphics_api::DescriptorPool create_descriptor_pool() const;

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const;

   void write_commands();

 private:
   void write_descriptors(graphics_api::PipelineStageFlags stages, BindingIndex index, const DescriptorInfo& info);
   void handle_descriptor_bindings();
   graphics_api::DescriptorArray& allocate_descriptors(DescriptorStorage& storage, graphics_api::DescriptorPool& pool) const;
   void write_descriptor(ResourceStorage& storage, const graphics_api::DescriptorView& descView,
                         const detail::cmd::BindDescriptors& descriptors) const;

   void add_texture_flag(Name texName, graphics_api::TextureUsage flag);
   void add_buffer_flag(Name buffName, graphics_api::BufferUsage flag);
   void set_buffer_in_transfer_state(Name buffName, bool value, graphics_api::PipelineStage pipelineStage);
   [[nodiscard]] bool is_buffer_in_transfer_state(Name buffName) const;
   void setup_transition(Name texName, graphics_api::TextureState targetState, graphics_api::PipelineStage targetStage);
   void setup_buffer_barrier(Name buffName, graphics_api::PipelineStage targetStage);
   graphics_api::RenderingInfo create_rendering_info(ResourceStorage& storage, const detail::cmd::BeginRenderPass& beginRenderPass) const;
   [[nodiscard]] bool is_depth_target(Name rtName) const;

   template<typename TDesc, typename... TArgs>
   void set_descriptor(const BindingIndex index, TArgs&&... args)
   {
      if (m_descriptors.size() <= index) {
         m_descriptors.resize(index + 1);
      }

      m_descriptors[index].emplace(std::in_place_type_t<TDesc>{}, std::forward<TArgs>(args)...);
   }

   template<typename TCmd, typename... TArgs>
   void add_command(TArgs&&... args)
   {
      m_commands.emplace_back(std::in_place_type_t<TCmd>{}, std::forward<TArgs>(args)...);
   }

   template<typename TDecl>
   const TDecl& declaration(const Name name) const
   {
      return std::get<TDecl>(m_declarations.at(name));
   }

   template<typename TDecl>
   TDecl& declaration(const Name name)
   {
      return std::get<TDecl>(m_declarations.at(name));
   }

   [[nodiscard]] bool is_render_target(Name texName) const;

   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;

   std::map<Name, detail::Declaration> m_declarations;
   std::vector<detail::Command> m_commands;

   graphics_api::WorkTypeFlags m_workTypes{};
   GraphicPipelineState m_graphicPipelineState{};
   ComputePipelineState m_computePipelineState{};
   DescriptorState m_descriptorState{};
   detail::DescriptorCounts m_descriptorCounts{};
   graphics_api::PipelineStage m_activePipelineStage{};

   std::vector<std::optional<detail::Descriptor>> m_descriptors;

   graphics_api::Pipeline* m_currentPipeline{};
};

class RenderPassScope
{
 public:
   template<typename... TRenderTargets>
   RenderPassScope(BuildContext& buildContext, Name passName, TRenderTargets&&... renderTargets) :
       m_buildContext(buildContext)
   {
      m_buildContext.begin_render_pass(passName, std::forward<TRenderTargets>(renderTargets)...);
   }

   ~RenderPassScope()
   {
      m_buildContext.end_render_pass();
   }

   RenderPassScope(const RenderPassScope& other) = delete;
   RenderPassScope(RenderPassScope&& other) noexcept = delete;
   RenderPassScope& operator=(const RenderPassScope& other) = delete;
   RenderPassScope& operator=(RenderPassScope&& other) noexcept = delete;

 private:
   BuildContext& m_buildContext;
};

}// namespace triglav::render_core
