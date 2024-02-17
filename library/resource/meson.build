resource_sources = files([
  'src/ResourceManager.cpp',
  'src/TextureLoader.cpp',
  'src/ShaderLoader.cpp',
  'src/MeshLoader.cpp',
])

resource_deps = [glm, graphics_api, core, geometry, font, fonts]
resource_incl = include_directories(['include', 'include/triglav/resource'])

resource_lib = static_library('resource',
  sources: resource_sources,
  dependencies: resource_deps,
  include_directories: resource_incl,
)

resource = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: resource_lib,
  dependencies: resource_deps,
)

subdir('test')