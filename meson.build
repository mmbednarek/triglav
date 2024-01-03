project('vulkan_engine',
  ['cpp', 'c'],
  version: '0.0.1',
  license: 'GPL3',
  meson_version: '>= 0.64',
  default_options: ['cpp_std=c++23'],
)

fs = import('fs')

vulkan = dependency('vulkan')
fmt = dependency('fmt')
glm = dependency('glm')
cgal = dependency('CGAL')
wayland_client = dependency('wayland-client')
xkb_common = dependency('xkbcommon')
freetype = dependency('freetype2')

subdir('fonts')
subdir('shader')
subdir('texture')
subdir('model')

subdir('library/external/mikktspace')
subdir('library/font')
subdir('library/graphics_api')
subdir('library/geometry')
subdir('library/renderer')

if host_machine.system() == 'windows'
  subdir('game/win32')
else
  subdir('game/linux_wayland')
endif

