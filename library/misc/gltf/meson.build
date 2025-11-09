gltf_sources = files([
  'include/triglav/gltf/BufferManager.hpp',
  'include/triglav/gltf/Glb.hpp',
  'include/triglav/gltf/Gltf.hpp',
  'include/triglav/gltf/MeshLoad.hpp',
  'src/BufferManager.cpp',
  'src/Glb.cpp',
  'src/Gltf.cpp',
  'src/MeshLoad.cpp',
])

gltf_deps_pub = [core, io, geometry]
gltf_deps_priv = [core, json_util, io, geometry, render_objects]
gltf_incl = include_directories(['include', 'include/triglav/gltf'])

gltf_lib = static_library('gltf',
  sources: gltf_sources,
  dependencies: gltf_deps_priv,
  include_directories: gltf_incl,
)

gltf = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: gltf_lib,
  dependencies: gltf_deps_pub,
)

subdir('test')