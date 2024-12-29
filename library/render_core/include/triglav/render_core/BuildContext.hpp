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

using TextureRef = std::variant<TextureName, Name>;
using BufferRef = std::variant<graphics_api::Buffer*, Name>;

namespace detail {

namespace descriptor {

struct RWTexture
{
   TextureRef texRef;
};

struct SamplableTexture
{
   TextureRef texRef;
};

struct UniformBuffer
{
   BufferRef buffRef{};
};

struct UniformBufferArray
{
   std::vector<BufferRef> buffers{};
};

}// namespace descriptor

using Descriptor =
   std::variant<descriptor::RWTexture, descriptor::SamplableTexture, descriptor::UniformBuffer, descriptor::UniformBufferArray>;

struct RenderTarget
{
   bool isScreenSize{true};
   bool isDepthTarget{false};
   graphics_api::ClearValue clearValue{};
   graphics_api::AttachmentAttributeFlags flags;
};

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

struct BindIndexBuffer
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

struct ExecutionBarrierData
{
   graphics_api::PipelineStageFlags srcStageFlags{};
   graphics_api::PipelineStageFlags dstStageFlags{};
};

struct ExecutionBarrier
{
   // hold the data in a unique ptr so it can be modified.
   std::unique_ptr<ExecutionBarrierData> data;
};

struct DrawPrimitives
{
   u32 vertexCount{};
   u32 vertexOffset{};
   u32 instanceCount{};
   u32 instanceOffset{};
};

struct DrawIndexedPrimitives
{
   u32 indexCount{};
   u32 indexOffset{};
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
   TextureRef srcTexture;
   BufferRef dstBuffer;
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

namespace decl {

struct Texture
{
   Name texName{};
   Vector2i texDims{};
   graphics_api::ColorFormat texFormat{};
   graphics_api::TextureUsageFlags texUsageFlags{};
   graphics_api::TextureState currentState{graphics_api::TextureState::Undefined};
   graphics_api::PipelineStage lastStage{graphics_api::PipelineStage::Entrypoint};
   graphics_api::PipelineStage lastWriteStage{graphics_api::PipelineStage::Entrypoint};
};

struct Buffer
{
   Name buffName{};
   MemorySize buffSize{};
   graphics_api::BufferUsageFlags buffUsageFlags{};
   graphics_api::PipelineStageFlags lastStages{};
   graphics_api::PipelineStage lastWriteStage{graphics_api::PipelineStage::Entrypoint};
   BufferState bufferState{BufferState::Undefined};
   cmd::ExecutionBarrierData* executionBarrier{};
};

}// namespace decl

using Declaration = std::variant<decl::Texture, decl::Buffer>;

using Command = std::variant<cmd::BindGraphicsPipeline, cmd::BindComputePipeline, cmd::DrawPrimitives, cmd::DrawIndexedPrimitives,
                             cmd::Dispatch, cmd::BindDescriptors, cmd::BindVertexBuffer, cmd::BindIndexBuffer, cmd::CopyTextureToBuffer,
                             cmd::TextureTransition, cmd::ExecutionBarrier, cmd::FillBuffer, cmd::BeginRenderPass, cmd::EndRenderPass>;

struct DescriptorCounts
{
   u32 storageTextureCount{};
   u32 uniformBufferCount{};
   u32 samplableTextureCount{};
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
   void declare_depth_target_with_dims(Name dtName, Vector2i dtDims, graphics_api::ColorFormat rtFormat = GAPI_FORMAT(D, UNorm16));
   void declare_buffer(Name buffName, MemorySize size);
   void declare_staging_buffer(Name buffName, MemorySize size);

   // Shader binding
   void bind_vertex_shader(VertexShaderName vsName);
   void bind_fragment_shader(FragmentShaderName fsName);
   void bind_compute_shader(ComputeShaderName csName);

   // Descriptor binding
   void bind_rw_texture(BindingIndex index, TextureRef texRef);
   void bind_samplable_texture(BindingIndex index, TextureRef texRef);
   void bind_uniform_buffer(BindingIndex index, BufferRef buffRef);
   void bind_uniform_buffers(BindingIndex index, std::span<const BufferRef> buffers);

   // Vertex binding
   void bind_vertex_layout(const VertexLayout& layout);
   void bind_vertex_buffer(Name buffName);
   void bind_index_buffer(Name buffName);

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
   void set_vertex_topology(graphics_api::VertexTopology topology);

   void draw_primitives(u32 vertexCount, u32 vertexOffset);
   void draw_indexed_primitives(u32 indexCount, u32 indexOffset, u32 vertexOffset);
   void draw_indexed_primitives(u32 indexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount, u32 instanceOffset);
   void draw_full_screen_quad();

   // Execution
   void dispatch(Vector3i dims);

   // Transfer
   void fill_buffer_raw(Name buffName, const void* ptr, MemorySize size);
   void copy_texture_to_buffer(TextureRef srcTex, BufferRef dstBuff);

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
   void set_pipeline_state_descriptor(graphics_api::PipelineStageFlags stages, BindingIndex index, const DescriptorInfo& info);
   void handle_descriptor_bindings();
   graphics_api::DescriptorArray& allocate_descriptors(DescriptorStorage& storage, graphics_api::DescriptorPool& pool) const;
   void write_descriptor(ResourceStorage& storage, const graphics_api::DescriptorView& descView,
                         const detail::cmd::BindDescriptors& descriptors) const;

   void add_texture_flag(Name texName, graphics_api::TextureUsage flag);
   void add_buffer_flag(Name buffName, graphics_api::BufferUsage flag);
   void setup_transition(Name texName, graphics_api::TextureState targetState, graphics_api::PipelineStage targetStage,
                         std::optional<graphics_api::PipelineStage> lastUsedStage = std::nullopt);
   void setup_buffer_barrier(Name buffName, BufferState targetState, graphics_api::PipelineStage targetStage);
   graphics_api::RenderingInfo create_rendering_info(ResourceStorage& storage, const detail::cmd::BeginRenderPass& beginRenderPass) const;
   graphics_api::Texture& resolve_texture_ref(ResourceStorage& storage, TextureRef texRef) const;
   graphics_api::Buffer& resolve_buffer_ref(ResourceStorage& storage, BufferRef buffRef) const;
   void handle_pending_graphic_state();
   void prepare_texture(Name texName, graphics_api::TextureState state, graphics_api::TextureUsage usage);
   void prepare_buffer(Name buffName, BufferState bufferState, graphics_api::BufferUsage usage);

   template<typename TDesc, typename... TArgs>
   void set_descriptor(const BindingIndex index, TArgs&&... args)
   {
      if (m_descriptors.size() <= index) {
         m_descriptors.resize(index + 1);
      }

      m_descriptors[index].emplace(std::in_place_type_t<TDesc>{}, std::forward<TArgs>(args)...);
   }

   template<typename TCmd, typename... TArgs>
   TCmd& add_command(TArgs&&... args)
   {
      m_commands.emplace_back(std::in_place_type_t<TCmd>{}, std::forward<TArgs>(args)...);
      return std::get<TCmd>(m_commands.back());
   }

   template<typename TCmd, typename... TArgs>
   TCmd& add_command_before_render_pass(TArgs&&... args)
   {
      if (!m_isWithinRenderPass) {
         return this->add_command<TCmd>(std::forward<TArgs>(args)...);
      }

      auto it = std::find_if(m_commands.rbegin(), m_commands.rend(),
                             [](const auto& cmd) { return std::holds_alternative<detail::cmd::BeginRenderPass>(cmd); });
      return std::get<TCmd>(*m_commands.emplace(it.base() - 1, std::in_place_type_t<TCmd>{}, std::forward<TArgs>(args)...));
   }

   template<typename TDecl, typename... TArgs>
   void add_declaration(Name declName, TArgs&&... args)
   {
      m_declarations.emplace(declName, detail::Declaration{std::in_place_type_t<TDecl>{}, declName, std::forward<TArgs>(args)...});
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

   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;

   std::map<Name, detail::RenderTarget> m_renderTargets;
   std::map<Name, detail::Declaration> m_declarations;
   std::vector<detail::Command> m_commands;

   graphics_api::WorkTypeFlags m_workTypes{};
   GraphicPipelineState m_graphicPipelineState{};
   ComputePipelineState m_computePipelineState{};
   DescriptorState m_descriptorState{};
   detail::DescriptorCounts m_descriptorCounts{};
   graphics_api::PipelineStage m_activePipelineStage{};
   bool m_isWithinRenderPass{false};

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
