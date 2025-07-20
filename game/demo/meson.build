demo_sources = files([
  'src/GameInstance.cpp',
  'src/GameInstance.hpp',
  'src/Main.cpp',
  'src/SplashScreen.cpp',
  'src/SplashScreen.hpp',
])

demo_deps = [
  renderer,
  geometry,
  desktop,
  threading,
  fmt,
  shaders,
]

demo = executable('demo',
  sources: demo_sources,
  dependencies: demo_deps,
  install: true,
  install_dir: 'bin',
  win_subsystem: 'windows',
  link_whole: [desktop_main_lib],
)
