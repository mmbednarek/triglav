demo_sources = files([
  'src/Main.cpp',
])

demo_deps = [
  renderer,
  geometry,
  desktop,
  fmt,
  xkb_common,
  shaders,
  textures,
  models,
  materials
]

demo = executable('demo',
  sources: demo_sources,
  dependencies: demo_deps,
  install: true,
  install_dir: 'bin'
)
