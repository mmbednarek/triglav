geometry_sources = files([
  'src/DebugMesh.cpp',
  'src/InternalMesh.cpp',
  'src/InternalMesh.h',
  'src/Mesh.cpp',
  'src/Parser.cpp',
])

geometry_pub_deps = [glm, graphics_api, io]

geometry_priv_deps = geometry_pub_deps
geometry_priv_deps += cgal
geometry_priv_deps += mikktspace

geometry_incl = include_directories(['include', 'include/triglav/geometry'])

geometry_lib = static_library('geometry',
  sources: geometry_sources,
  dependencies: geometry_priv_deps,
  include_directories: geometry_incl,
)

geometry = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: geometry_lib,
  dependencies: geometry_pub_deps,
)
