demo_sources = files([
  'src/Main.cpp',
  'src/GameInstance.cpp',
  'src/GameInstance.h',
])

demo_deps = [
  renderer,
  geometry,
  desktop,
  desktop_main,
  threading,
  fmt,
  shaders,
]

demo_link_args = ''

if host_machine.system() == 'windows'
  demo_link_args = '/SUBSYSTEM:WINDOWS'
endif

demo = executable('demo',
  sources: demo_sources,
  dependencies: demo_deps,
  install: true,
  install_dir: 'bin',
  link_args: demo_link_args,
)
