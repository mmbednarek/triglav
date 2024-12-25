render_objects_sources = files([
  'include/triglav/render_objects/Material.hpp',
  'include/triglav/render_objects/Model.hpp',
  'include/triglav/render_objects/RenderObjects.hpp',
  'src/RenderObjects.cpp'
])

render_objects_deps = [glm, graphics_api, core, geometry]
render_objects_incl = include_directories(['include', 'include/triglav/render_objects'])

render_objects_lib = static_library('render_objects',
  sources: render_objects_sources,
  dependencies: render_objects_deps,
  include_directories: render_objects_incl,
)

render_objects = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: render_objects_lib,
  dependencies: render_objects_deps,
)
