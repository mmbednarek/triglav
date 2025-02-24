triglavcli_sources = files([
  'src/Commands.hpp',
  'src/Commands.cpp',
  'src/HelpHandler.cpp',
  'src/ImportHandler.cpp',
  'src/InspectHandler.cpp',
  'src/Main.cpp',
])

triglavcli_deps = [
  geometry,
  fmt,
  asset,
  gltf,
  stbi,
  graphics_api
]

triglavcli = executable('triglavcli',
  sources: triglavcli_sources,
  dependencies: triglavcli_deps,
  install: true,
  install_dir: 'bin',
)
