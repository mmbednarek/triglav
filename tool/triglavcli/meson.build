triglavcli_sources = files([
  'src/Commands.hpp',
  'src/ImportHandler.cpp',
  'src/InspectHandler.cpp',
  'src/Main.cpp',
])

triglavcli_deps = [
  geometry,
  fmt,
  asset,
  gltf
]

triglavcli = executable('triglavcli',
  sources: triglavcli_sources,
  dependencies: triglavcli_deps,
  install: true,
  install_dir: 'bin',
)
