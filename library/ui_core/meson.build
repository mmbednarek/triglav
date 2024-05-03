ui_core_sources = files([
  'include/triglav/ui_core/Primitives.hpp',
  'include/triglav/ui_core/Viewport.h',
  'src/Viewport.cpp',
])

ui_core_deps = [glm, geometry, font, fonts, io, resource, core]
ui_core_incl = include_directories(['include', 'include/triglav/ui_core'])

ui_core_lib = static_library('ui_core',
  sources: ui_core_sources,
  dependencies: ui_core_deps,
  include_directories: ui_core_incl,
)

ui_core = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: [ui_core_lib],
  dependencies: ui_core_deps,
)
