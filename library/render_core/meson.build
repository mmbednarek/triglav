render_core_sources = files([
  'src/RenderCore.cpp',
  'src/RenderGraph.cpp',
])

render_core_deps = [glm, graphics_api, core, geometry, font, fonts, io]
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
