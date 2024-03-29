demo_sources = files([
  'src/Main.cpp',
])

demo_deps = [
  renderer,
  geometry,
  desktop,
  desktop_main,
  fmt,
  xkb_common,
  shaders,
  textures,
  models,
  materials,
  levels
]

demo = executable('demo',
  sources: demo_sources,
  dependencies: demo_deps,
  install: true,
  install_dir: 'bin'
)
