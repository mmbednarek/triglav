renderer_sources = files([
  'src/AssetMap.hpp',
  'src/Context2D.cpp',
  'src/Context3D.cpp',
  'src/Material.cpp',
  'src/Model.cpp',
  'src/Renderer.cpp',
  'src/ResourceManager.cpp',
  'src/Scene.cpp',
  'src/ShadowMap.cpp',
  'src/SkyBox.cpp',
  'src/stb_image.h',
])

renderer_deps = [glm, graphics_api, geometry]
renderer_incl = include_directories(['include', 'include/renderer'])

renderer_lib = static_library('renderer',
  sources: renderer_sources,
  dependencies: renderer_deps,
  include_directories: renderer_incl,
)

renderer = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: renderer_lib,
  dependencies: renderer_deps,
)
