triglavcli_sources = files([
  'src/Commands.cpp',
  'src/Commands.hpp',
  'src/HelpHandler.cpp',
  'src/ImportHandler.cpp',
  'src/InspectHandler.cpp',
  'src/LevelImport.cpp',
  'src/LevelImport.hpp',
  'src/Main.cpp',
  'src/MeshImport.cpp',
  'src/MeshImport.hpp',
  'src/ProjectConfig.cpp',
  'src/ProjectConfig.hpp',
  'src/ProjectHandler.cpp',
  'src/ResourceList.cpp',
  'src/ResourceList.hpp',
  'src/TextureImport.hpp',
  'src/TextureImport.cpp',
])

triglavcli_deps = [
  geometry,
  fmt,
  asset,
  gltf,
  stbi,
  graphics_api,
  json_util,
  world,
  rapidyaml,
  render_objects
]

triglavcli = executable('triglavcli',
  sources: triglavcli_sources,
  dependencies: triglavcli_deps,
  install: true,
  install_dir: 'bin',
)
