project('vulkan_engine',
  ['cpp', 'c'],
  version: '0.0.1',
  license: 'GPL3',
  meson_version: '>= 1.1',
  default_options: ['cpp_std=c++23'],
)

fs = import('fs')

vulkan = dependency('vulkan', method:'pkg-config')
fmt = dependency('fmt', method:'pkg-config')
glm = dependency('glm', method:'pkg-config')
cgal = dependency('cgal', method:'pkg-config')
freetype = dependency('freetype2', method:'pkg-config')
gtest = dependency('gtest', method:'pkg-config')
rapidyaml = dependency('rapidyaml', method:'pkg-config')
spdlog = dependency('spdlog', method:'pkg-config')

if build_machine.system() == 'linux'
  xlib = dependency('x11')
  xkb_common = dependency('xkbcommon')
  wayland_client = dependency('wayland-client')
endif

subdir('fonts')
subdir('shader')
subdir('texture')
subdir('model')
subdir('material')
subdir('level')

subdir('library/external/mikktspace')

subdir('library/core')
subdir('library/io')

subdir('library/world')
subdir('library/desktop')
subdir('library/desktop_main')
subdir('library/font')
subdir('library/graphics_api')
subdir('library/geometry')
subdir('library/render_core')
subdir('library/resource')

subdir('library/renderer')

subdir('game/demo')

