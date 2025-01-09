render_core_sources = files([
  'include/triglav/render_core/BuildContext.hpp',
  'include/triglav/render_core/FrameResources.hpp',
  'include/triglav/render_core/GlyphAtlas.hpp',
  'include/triglav/render_core/IRenderNode.hpp',
  'include/triglav/render_core/Job.hpp',
  'include/triglav/render_core/JobGraph.hpp',
  'include/triglav/render_core/PipelineCache.hpp',
  'include/triglav/render_core/RenderCore.hpp',
  'include/triglav/render_core/RenderGraph.hpp',
  'include/triglav/render_core/ResourceStorage.hpp',
  'src/BuildContext.cpp',
  'src/FrameResources.cpp',
  'src/GlyphAtlas.cpp',
  'src/Job.cpp',
  'src/JobGraph.cpp',
  'src/PipelineCache.cpp',
  'src/RenderCore.cpp',
  'src/RenderGraph.cpp',
  'src/ResourceStorage.cpp',
])

render_core_deps = [glm, graphics_api, core, geometry, font, io, spdlog, resource]
render_core_incl = include_directories(['include', 'include/triglav/render_core'])

render_core_lib = static_library('render_core',
  sources: render_core_sources,
  dependencies: render_core_deps,
  include_directories: render_core_incl,
)

render_core = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: render_core_lib,
  dependencies: render_core_deps,
)

subdir('test')
