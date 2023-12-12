linux_wayland_sources = files([
  'src/Main.cpp',
  'src/wayland/Display.cpp',
  'src/wayland/Display.h',
  'src/wayland/ISurfaceEventListener.hpp',
  'src/wayland/Surface.cpp',
  'src/wayland/Surface.h',
  'src/wayland/api/PointerConstraints.c',
  'src/wayland/api/PointerConstraints.h',
  'src/wayland/api/RelativePointer.c',
  'src/wayland/api/RelativePointer.h',
  'src/wayland/api/XdgShell.c',
  'src/wayland/api/XdgShell.h',
])

linux_wayland_deps = [
  renderer,
  geometry,
  wayland_client,
  fmt,
  xkb_common,
  shaders,
  textures,
  models,
]

linux_wayland = executable('linux_wayland',
  sources: linux_wayland_sources,
  dependencies: linux_wayland_deps,
  install: true,
  install_dir: 'bin'
)
