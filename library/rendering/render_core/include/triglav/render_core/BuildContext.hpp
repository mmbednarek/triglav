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
   graphics_api::ClearValue clear_value{};
   graphics_api::AttachmentAttributeFlags flags;
};

struct DescriptorCounts
{
   u32 storage_texture_count{};
   u32 uniform_buffer_count{};
   u32 storage_buffer_count{};
   u32 sampled_texture_count{};
   u32 texture_count{};
   u32 acceleration_structure_count{};
   u32 total_descriptor_sets{};
};

}// namespace detail

class BuildContext
{
   friend class GenerateCommandListPass;
   friend class BarrierInsertionPass;

 public:
   BuildContext(graphics_api::Device& device, resource::ResourceManager& resource_manager, Vector2i screen_size);

   // Declarations
   void declare_texture(Name tex_name, Vector2i tex_dims, graphics_api::ColorFormat tex_format = GAPI_FORMAT(RGBA, sRGB));
   void declare_screen_size_texture(Name tex_name, graphics_api::ColorFormat tex_format = GAPI_FORMAT(RGBA, sRGB));
   void declare_proportional_texture(Name tex_name, graphics_api::ColorFormat tex_format, float scale,
                                     bool create_mip_levels);// texture as a proportion of screen size
   void declare_render_target(Name rt_name, graphics_api::ColorFormat rt_format = GAPI_FORMAT(RGBA, UNorm8));
   void declare_depth_target(Name dt_name, graphics_api::ColorFormat rt_format = GAPI_FORMAT(RGBA, UNorm8));
   void declare_proportional_depth_target(Name dt_name, graphics_api::ColorFormat rt_format = GAPI_FORMAT(RGBA, UNorm8),
                                          float scale = 1.0f);
   void declare_sized_render_target(Name rt_name, Vector2i rt_dims, graphics_api::ColorFormat rt_format = GAPI_FORMAT(RGBA, UNorm8));
   void declare_sized_depth_target(Name dt_name, Vector2i dt_dims, graphics_api::ColorFormat rt_format = GAPI_FORMAT(D, UNorm16));
   void declare_buffer(Name buff_name, MemorySize size);
   void declare_staging_buffer(Name buff_name, MemorySize size);
   void declare_proportional_buffer(Name buff_name, float scale, MemorySize stride);

   // Shader binding
   void bind_vertex_shader(VertexShaderName vs_name);
   void bind_fragment_shader(FragmentShaderName fs_name);
   void bind_compute_shader(ComputeShaderName cs_name);

   // Descriptor binding
   void bind_rw_texture(BindingIndex index, TextureRef tex_ref);
   void bind_samplable_texture(BindingIndex index, TextureRef tex_ref);
   void bind_texture(BindingIndex index, TextureRef tex_ref);
   void bind_sampled_texture_array(BindingIndex index, std::span<const TextureRef> tex_refs);
   void bind_uniform_buffer(BindingIndex index, BufferRef buff_ref);
   void bind_uniform_buffer(BindingIndex index, BufferRef buff_ref, u32 offset, u32 size);
   void bind_uniform_buffers(BindingIndex index, std::span<const BufferRef> buffers);
   void bind_storage_buffer(BindingIndex index, BufferRef buff_ref);

   // Push constant
   void push_constant_span(std::span<const u8> buffer);

   template<typename T>
   void push_constant(const T& value)
   {
      this->push_constant_span({reinterpret_cast<const u8*>(&value), sizeof(std::decay_t<T>)});
   }

   // Vertex binding
   void bind_vertex_layout(const VertexLayout& layout);
   void bind_vertex_buffer(BufferRef buff_ref);
   void bind_index_buffer(BufferRef buff_ref);

   // Render pass
   void begin_render_pass_raw(Name pass_name, std::span<Name> render_targets);
   void end_render_pass();
   void clear_color(Name target_name, Vector4 color);
   void clear_depth_stencil(Name target_name, float depth, u32 stencil);

   // Queries
   void reset_timestamp_queries(u32 offset, u32 count);
   void reset_pipeline_queries(u32 offset, u32 count);
   void query_timestamp(u32 index, bool is_closing);
   void begin_query(u32 index);
   void end_query(u32 index);

   template<typename... TArgs>
   void begin_render_pass(Name pass_name, TArgs... args)
   {
      std::array<Name, sizeof...(TArgs)> render_targets{args...};
      this->begin_render_pass_raw(pass_name, render_targets);
   }

   // Drawing
   void set_vertex_topology(graphics_api::VertexTopology topology);
   void set_depth_test_mode(graphics_api::DepthTestMode mode);
   void set_is_blending_enabled(bool enabled);
   void set_viewport(Vector4 dimensions, float min_depth, float max_depth);
   void set_line_width(float width);

   void draw_primitives(u32 vertex_count, u32 vertex_offset, u32 instance_count = 1, u32 instance_offset = 0);
   void draw_indexed_primitives(u32 index_count, u32 index_offset, u32 vertex_offset);
   void draw_indexed_primitives(u32 index_count, u32 index_offset, u32 vertex_offset, u32 instance_count, u32 instance_offset);
   void draw_indexed_indirect_with_count(BufferRef draw_call_buffer, BufferRef count_buffer, u32 max_draw_calls, u32 stride,
                                         u32 count_buffer_offset = 0);
   void draw_indirect_with_count(BufferRef draw_call_buffer, BufferRef count_buffer, u32 max_draw_calls, u32 stride);
   void draw_full_screen_quad();

   // Execution
   void dispatch(Vector3i dims);
   void dispatch_indirect(BufferRef indirect_buffer);

   // Transfer
   void fill_buffer_raw(Name buff_name, const void* ptr, MemorySize size);
   void init_buffer_raw(Name buff_name, const void* ptr, MemorySize size);
   void copy_texture_to_buffer(TextureRef src_tex, BufferRef dst_buff);
   void copy_buffer_to_texture(BufferRef src_buff, TextureRef dst_tex);
   void copy_buffer(BufferRef src_buffer, BufferRef dst_buffer);
   void copy_texture(TextureRef src_tex, TextureRef dst_tex);
   void copy_texture_region(TextureRef src_tex, Vector2i src_offset, TextureRef dst_tex, Vector2i dst_offset, Vector2i size);
   void blit_texture(TextureRef src_tex, TextureRef dst_tex);

   // Conditional commands
   void declare_flag(Name flag_name);
   void if_enabled(Name flag);
   void if_disabled(Name flag);
   void end_if();

   // Ray tracing
   void bind_rt_generation_shader(RayGenShaderName ray_gen_shader);
   void bind_rt_closest_hit_shader(RayClosestHitShaderName ray_closest_hit_shader);
   void bind_rt_miss_shader(RayMissShaderName ray_miss_shader);
   void bind_rt_shader_group(RayTracingShaderGroup group);
   void set_rt_max_recursion_depth(u32 max_recursion_depth);
   void bind_acceleration_structure(u32 index, graphics_api::ray_tracing::AccelerationStructure& acceleration_structure);
   void trace_rays(Vector3i dimensions);

   template<typename TData>
   void fill_buffer(const Name buff_name, const TData& data)
   {
      this->fill_buffer_raw(buff_name, reinterpret_cast<const void*>(&data), sizeof(data));
   }

   template<typename TData>
   void init_buffer(const Name buff_name, const TData& data)
   {
      this->declare_buffer(buff_name, sizeof(TData));
      this->fill_buffer(buff_name, data);
   }

   Job build_job(PipelineCache& pipeline_cache, ResourceStorage& storage, Name job_name = {});

   void write_commands(ResourceStorage& storage, DescriptorStorage& desc_storage, graphics_api::CommandList& cmd_list, PipelineCache& cache,
                       graphics_api::DescriptorPool* pool, u32 frame_index, u32 enabled_flags);

   void create_resources(ResourceStorage& storage);
   [[nodiscard]] std::optional<graphics_api::DescriptorPool> create_descriptor_pool() const;

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const;
   [[nodiscard]] Vector2i screen_size() const;

   void export_texture(Name tex_name, graphics_api::PipelineStage pipeline_stage, graphics_api::TextureState state,
                       graphics_api::TextureUsageFlags flags);
   void export_buffer(Name buff_name, graphics_api::PipelineStage pipeline_stage, graphics_api::BufferAccess access,
                      graphics_api::BufferUsageFlags flags);

   [[nodiscard]] graphics_api::SamplerProperties& texture_sampler_properties(Name name);
   void set_sampler_properties(Name name, const graphics_api::SamplerProperties& sampler_props);

   template<typename TVertex>
   void draw_mesh(const graphics_api::Mesh<TVertex>& mesh)
   {
      this->bind_vertex_buffer(&mesh.vertices.buffer());
      this->bind_index_buffer(&mesh.indices.buffer());
      this->draw_indexed_primitives(static_cast<u32>(mesh.indices.count()), 0, 0);
   }

   void add_texture_usage(Name tex_name, graphics_api::TextureUsageFlags flags);
   void add_buffer_usage(Name buff_name, graphics_api::BufferUsageFlags flags);
   void set_bind_stages(graphics_api::PipelineStageFlags stages);
   [[nodiscard]] bool is_ray_tracing_supported() const;


 private:
   void set_pipeline_state_descriptor(graphics_api::PipelineStageFlags stages, BindingIndex index, const DescriptorInfo& info);
   void handle_descriptor_bindings();
   graphics_api::DescriptorArray& allocate_descriptors(DescriptorStorage& storage, graphics_api::DescriptorPool& pool,
                                                       graphics_api::Pipeline& pipeline) const;
   void write_descriptor(ResourceStorage& storage, const graphics_api::DescriptorView& desc_view,
                         const detail::cmd::BindDescriptors& descriptors, u32 frame_index) const;

   graphics_api::RenderingInfo create_rendering_info(ResourceStorage& storage, const detail::cmd::BeginRenderPass& begin_render_pass,
                                                     u32 frame_index) const;
   const graphics_api::Texture& resolve_texture_ref(ResourceStorage& storage, TextureRef tex_ref, u32 frame_index) const;
   const graphics_api::TextureView& resolve_texture_view_ref(ResourceStorage& storage, TextureRef tex_ref, u32 frame_index) const;
   const graphics_api::Buffer& resolve_buffer_ref(ResourceStorage& storage, BufferRef buff_ref, u32 frame_index) const;
   void handle_pending_graphic_state();

   // Barrier support
   void prepare_texture(TextureRef tex_ref, graphics_api::TextureState state, graphics_api::TextureUsageFlags usage);
   void prepare_buffer(BufferRef buff_ref, graphics_api::BufferUsage usage);
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
   void add_declaration(Name decl_name, TArgs&&... args)
   {
      m_declarations.emplace(decl_name, detail::Declaration{std::in_place_type_t<TDecl>{}, decl_name, std::forward<TArgs>(args)...});
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
   resource::ResourceManager& m_resource_manager;

   std::map<Name, detail::RenderTarget> m_render_targets;
   std::map<Name, detail::Declaration> m_declarations;
   std::vector<detail::Command> m_commands;

   graphics_api::WorkTypeFlags m_work_types{};
   GraphicPipelineState m_graphic_pipeline_state{};
   ComputePipelineState m_compute_pipeline_state{};
   RayTracingPipelineState m_ray_tracing_pipeline_state{};
   DescriptorState m_descriptor_state{};
   detail::DescriptorCounts m_descriptor_counts{};
   graphics_api::PipelineStageFlags m_active_pipeline_stages{};
   Vector2i m_screen_size{};
   std::vector<Name> m_flags;
   std::deque<std::tuple<u32, bool>> m_flag_stack;
   std::vector<detail::cmd::PushConstant> m_pending_push_constants;

   std::vector<std::optional<detail::DescriptorAndStage>> m_descriptors;
};

class RenderPassScope
{
 public:
   template<typename... TRenderTargets>
   RenderPassScope(BuildContext& build_context, Name pass_name, TRenderTargets&&... render_targets) :
       m_build_context(build_context)
   {
      m_build_context.begin_render_pass(pass_name, std::forward<TRenderTargets>(render_targets)...);
   }

   ~RenderPassScope()
   {
      m_build_context.end_render_pass();
   }

   RenderPassScope(const RenderPassScope& other) = delete;
   RenderPassScope(RenderPassScope&& other) noexcept = delete;
   RenderPassScope& operator=(const RenderPassScope& other) = delete;
   RenderPassScope& operator=(RenderPassScope&& other) noexcept = delete;

 private:
   BuildContext& m_build_context;
};

template<typename TVisitor, typename TCommand>
concept HasOverrideForCommand = requires(TVisitor visitor, TCommand command) {
   { visitor.visit(command) };
};

template<typename TVisitor>
void visit_command(TVisitor& visitor, const detail::Command& cmd_variant)
{
   std::visit(
      [&visitor, &cmd_variant]<typename TCommand>(TCommand& cmd) {
         if constexpr (HasOverrideForCommand<TVisitor, TCommand>) {
            visitor.visit(cmd);
         } else {
            visitor.default_visit(cmd_variant);
         }
      },
      cmd_variant);
}

}// namespace triglav::render_core
