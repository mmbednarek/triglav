#pragma once

#include "Job.hpp"
#include "PipelineCache.hpp"
#include "RenderCore.hpp"
#include "ResourceStorage.hpp"
#include "detail/Commands.hpp"
#include "detail/Declarations.hpp"
#include "detail/Descriptors.hpp"

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/graphics_api/GraphicsApi.hpp"

#include <deque>
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

struct RenderTarget
{
   graphics_api::ClearValue clearValue{};
   graphics_api::AttachmentAttributeFlags flags;
};

struct DescriptorCounts
{
   u32 storageTextureCount{};
   u32 uniformBufferCount{};
   u32 storageBufferCount{};
   u32 sampledTextureCount{};
   u32 textureCount{};
   u32 accelerationStructureCount{};
   u32 totalDescriptorSets{};
};

}// namespace detail

class BuildContext
{
   friend class GenerateCommandListPass;
   friend class BarrierInsertionPass;

 public:
   BuildContext(graphics_api::Device& device, resource::ResourceManager& resourceManager, Vector2i screenSize);

   // Declarations
   void declare_texture(Name texName, Vector2i texDims, graphics_api::ColorFormat texFormat = GAPI_FORMAT(RGBA, sRGB));
   void declare_screen_size_texture(Name texName, graphics_api::ColorFormat texFormat = GAPI_FORMAT(RGBA, sRGB));
   void declare_proportional_texture(Name texName, graphics_api::ColorFormat texFormat, float scale,
                                     bool createMipLevels);// texture as a proportion of screen size
   void declare_render_target(Name rtName, graphics_api::ColorFormat rtFormat = GAPI_FORMAT(RGBA, UNorm8));
   void declare_depth_target(Name dtName, graphics_api::ColorFormat rtFormat = GAPI_FORMAT(RGBA, UNorm8));
   void declare_proportional_depth_target(Name dtName, graphics_api::ColorFormat rtFormat = GAPI_FORMAT(RGBA, UNorm8), float scale = 1.0f);
   void declare_sized_render_target(Name rtName, Vector2i rtDims, graphics_api::ColorFormat rtFormat = GAPI_FORMAT(RGBA, UNorm8));
   void declare_sized_depth_target(Name dtName, Vector2i dtDims, graphics_api::ColorFormat rtFormat = GAPI_FORMAT(D, UNorm16));
   void declare_buffer(Name buffName, MemorySize size);
   void declare_staging_buffer(Name buffName, MemorySize size);
   void declare_proportional_buffer(Name buffName, float scale, MemorySize stride);

   // Shader binding
   void bind_vertex_shader(VertexShaderName vsName);
   void bind_fragment_shader(FragmentShaderName fsName);
   void bind_compute_shader(ComputeShaderName csName);

   // Descriptor binding
   void bind_rw_texture(BindingIndex index, TextureRef texRef);
   void bind_samplable_texture(BindingIndex index, TextureRef texRef);
   void bind_texture(BindingIndex index, TextureRef texRef);
   void bind_sampled_texture_array(BindingIndex index, std::span<const TextureRef> texRefs);
   void bind_uniform_buffer(BindingIndex index, BufferRef buffRef);
   void bind_uniform_buffers(BindingIndex index, std::span<const BufferRef> buffers);
   void bind_storage_buffer(BindingIndex index, BufferRef buffRef);

   // Push constant
   void push_constant_span(std::span<const u8> buffer);

   template<typename T>
   void push_constant(const T& value)
   {
      this->push_constant_span({reinterpret_cast<const u8*>(&value), sizeof(std::decay_t<T>)});
   }

   // Vertex binding
   void bind_vertex_layout(const VertexLayout& layout);
   void bind_vertex_buffer(BufferRef buffRef);
   void bind_index_buffer(BufferRef buffRef);

   // Render pass
   void begin_render_pass_raw(Name passName, std::span<Name> renderTargets);
   void end_render_pass();
   void clear_color(Name targetName, Vector4 color);

   // Queries
   void reset_timestamp_queries(u32 offset, u32 count);
   void reset_pipeline_queries(u32 offset, u32 count);
   void query_timestamp(u32 index, bool isClosing);
   void begin_query(u32 index);
   void end_query(u32 index);

   template<typename... TArgs>
   void begin_render_pass(Name passName, TArgs... args)
   {
      std::array<Name, sizeof...(TArgs)> renderTargets{args...};
      this->begin_render_pass_raw(passName, renderTargets);
   }

   // Drawing
   void set_vertex_topology(graphics_api::VertexTopology topology);
   void set_depth_test_mode(graphics_api::DepthTestMode mode);
   void set_is_blending_enabled(bool enabled);

   void draw_primitives(u32 vertexCount, u32 vertexOffset, u32 instanceCount = 1, u32 instanceOffset = 0);
   void draw_indexed_primitives(u32 indexCount, u32 indexOffset, u32 vertexOffset);
   void draw_indexed_primitives(u32 indexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount, u32 instanceOffset);
   void draw_indexed_indirect_with_count(BufferRef drawCallBuffer, BufferRef countBuffer, u32 maxDrawCalls, u32 stride);
   void draw_indirect_with_count(BufferRef drawCallBuffer, BufferRef countBuffer, u32 maxDrawCalls, u32 stride);
   void draw_full_screen_quad();

   // Execution
   void dispatch(Vector3i dims);
   void dispatch_indirect(BufferRef indirectBuffer);

   // Transfer
   void fill_buffer_raw(Name buffName, const void* ptr, MemorySize size);
   void copy_texture_to_buffer(TextureRef srcTex, BufferRef dstBuff);
   void copy_buffer_to_texture(BufferRef srcBuff, TextureRef dstTex);
   void copy_buffer(BufferRef srcBuffer, BufferRef dstBuffer);
   void copy_texture(TextureRef srcTex, TextureRef dstTex);

   // Conditional commands
   void declare_flag(Name flagName);
   void if_enabled(Name flag);
   void if_disabled(Name flag);
   void end_if();

   // Ray tracing
   void bind_rt_generation_shader(RayGenShaderName rayGenShader);
   void bind_rt_closest_hit_shader(RayClosestHitShaderName rayClosestHitShader);
   void bind_rt_miss_shader(RayMissShaderName rayMissShader);
   void bind_rt_shader_group(RayTracingShaderGroup group);
   void set_rt_max_recursion_depth(u32 maxRecursionDepth);
   void bind_acceleration_structure(u32 index, graphics_api::ray_tracing::AccelerationStructure& accelerationStructure);
   void trace_rays(Vector3i dimensions);

   template<typename TData>
   void fill_buffer(const Name buffName, const TData& data)
   {
      this->fill_buffer_raw(buffName, reinterpret_cast<const void*>(&data), sizeof(data));
   }

   template<typename TData>
   void init_buffer(const Name buffName, const TData& data)
   {
      this->declare_buffer(buffName, sizeof(TData));
      this->fill_buffer(buffName, data);
   }

   Job build_job(PipelineCache& pipelineCache, ResourceStorage& storage, Name jobName = {});

   void write_commands(ResourceStorage& storage, DescriptorStorage& descStorage, graphics_api::CommandList& cmdList, PipelineCache& cache,
                       graphics_api::DescriptorPool* pool, u32 frameIndex, u32 enabledFlags);

   void create_resources(ResourceStorage& storage);
   [[nodiscard]] std::optional<graphics_api::DescriptorPool> create_descriptor_pool() const;

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const;
   [[nodiscard]] Vector2i screen_size() const;

   void export_texture(Name texName, graphics_api::PipelineStage pipelineStage, graphics_api::TextureState state,
                       graphics_api::TextureUsageFlags flags);
   void export_buffer(Name buffName, graphics_api::PipelineStage pipelineStage, graphics_api::BufferAccess access,
                      graphics_api::BufferUsageFlags flags);

   [[nodiscard]] graphics_api::SamplerProperties& texture_sampler_properties(Name name);
   void set_sampler_properties(Name name, const graphics_api::SamplerProperties& samplerProps);

   template<typename TVertex>
   void draw_mesh(const graphics_api::Mesh<TVertex>& mesh)
   {
      this->bind_vertex_buffer(&mesh.vertices.buffer());
      this->bind_index_buffer(&mesh.indices.buffer());
      this->draw_indexed_primitives(mesh.indices.count(), 0, 0);
   }

   void add_texture_usage(Name texName, graphics_api::TextureUsageFlags flags);
   void add_buffer_usage(Name buffName, graphics_api::BufferUsageFlags flags);
   void set_bind_stages(graphics_api::PipelineStageFlags stages);
   [[nodiscard]] bool is_ray_tracing_supported() const;


 private:
   void set_pipeline_state_descriptor(graphics_api::PipelineStageFlags stages, BindingIndex index, const DescriptorInfo& info);
   void handle_descriptor_bindings();
   graphics_api::DescriptorArray& allocate_descriptors(DescriptorStorage& storage, graphics_api::DescriptorPool& pool,
                                                       graphics_api::Pipeline& pipeline) const;
   void write_descriptor(ResourceStorage& storage, const graphics_api::DescriptorView& descView,
                         const detail::cmd::BindDescriptors& descriptors, u32 frameIndex) const;

   graphics_api::RenderingInfo create_rendering_info(ResourceStorage& storage, const detail::cmd::BeginRenderPass& beginRenderPass,
                                                     u32 frameIndex) const;
   const graphics_api::Texture& resolve_texture_ref(ResourceStorage& storage, TextureRef texRef, u32 frameIndex) const;
   const graphics_api::TextureView& resolve_texture_view_ref(ResourceStorage& storage, TextureRef texRef, u32 frameIndex) const;
   const graphics_api::Buffer& resolve_buffer_ref(ResourceStorage& storage, BufferRef buffRef, u32 frameIndex) const;
   void handle_pending_graphic_state();

   // Barrier support
   void prepare_texture(TextureRef texRef, graphics_api::TextureState state, graphics_api::TextureUsageFlags usage);
   void prepare_buffer(BufferRef buffRef, graphics_api::BufferUsage usage);
   [[nodiscard]] u32 flag_variation_count() const;
   void reset_resource_states();

   template<typename TDesc, typename... TArgs>
   void set_descriptor(const BindingIndex index, const graphics_api::PipelineStageFlags stages, TArgs&&... args)
   {
      if (m_descriptors.size() <= index) {
         m_descriptors.resize(index + 1);
      }

      m_descriptors[index].emplace(
         detail::DescriptorAndStage{detail::Descriptor{std::in_place_type_t<TDesc>{}, std::forward<TArgs>(args)...}, stages});
   }

   template<typename TCmd, typename... TArgs>
   TCmd& add_command(TArgs&&... args)
   {
      m_commands.emplace_back(std::in_place_type_t<TCmd>{}, std::forward<TArgs>(args)...);
      return std::get<TCmd>(m_commands.back());
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
   RayTracingPipelineState m_rayTracingPipelineState{};
   DescriptorState m_descriptorState{};
   detail::DescriptorCounts m_descriptorCounts{};
   graphics_api::PipelineStageFlags m_activePipelineStages{};
   Vector2i m_screenSize{};
   std::vector<Name> m_flags;
   std::deque<std::tuple<u32, bool>> m_flagStack;
   std::vector<detail::cmd::PushConstant> m_pendingPushConstants;

   std::vector<std::optional<detail::DescriptorAndStage>> m_descriptors;
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

template<typename TVisitor, typename TCommand>
concept HasOverrideForCommand = requires(TVisitor visitor, TCommand command) {
   { visitor.visit(command) };
};

template<typename TVisitor>
void visit_command(TVisitor& visitor, const detail::Command& cmdVariant)
{
   std::visit(
      [&visitor, &cmdVariant]<typename TCommand>(TCommand& cmd) {
         if constexpr (HasOverrideForCommand<TVisitor, TCommand>) {
            visitor.visit(cmd);
         } else {
            visitor.default_visit(cmdVariant);
         }
      },
      cmdVariant);
}

}// namespace triglav::render_core
