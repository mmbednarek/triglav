triglavcli_sources = files([
  'src/Commands.cpp',
  'src/Commands.hpp',
  'src/HelpHandler.cpp',
  'src/ImportHandler.cpp',
  'src/InspectHandler.cpp',
  'src/Main.cpp',
  'src/ProjectConfig.cpp',
  'src/ProjectConfig.hpp',
  'src/ProjectHandler.cpp',
])

triglavcli_deps = [
  geometry,
  fmt,
  asset,
  gltf,
  stbi,
  graphics_api,
  json_util
]

triglavcli = executable('triglavcli',
  sources: triglavcli_sources,
  dependencies: triglavcli_deps,
  install: true,
  install_dir: 'bin',
)
