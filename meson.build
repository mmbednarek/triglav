project('vulkan_engine',
  ['cpp', 'c'],
  version: '0.0.1',
  license: 'GPL3',
  meson_version: '>= 1.5',
  default_options: ['cpp_std=c++23'],
)

if meson.get_compiler('cpp').get_id() == 'msvc'
  add_project_arguments('/std:c++latest', language : 'cpp')
  add_project_arguments('/Zc:preprocessor', language : 'cpp')
endif

if host_machine.system() == 'windows' and meson.get_compiler('cpp').get_id() == 'gcc'
  add_project_arguments('-Wa,-mbig-obj', language : 'cpp')
endif

if meson.get_compiler('cpp').get_id() == 'clang'
  if get_option('thread_sanitizer').enabled()
    add_project_arguments('-fsanitize=thread', language: 'cpp')
    add_project_link_arguments('-fsanitize=thread', language: 'cpp')
  elif get_option('address_sanitizer').enabled()
    add_project_arguments('-fsanitize=address', language: 'cpp')
    add_project_arguments('-fno-omit-frame-pointer', language: 'cpp')
    add_project_link_arguments('-fsanitize=address', language: 'cpp')
  endif
endif

fs = import('fs')

# Using system vulkan
vulkan = dependency('vulkan')
fmt = dependency('fmt', method:'pkg-config')
glm = dependency('glm', method:'pkg-config')
cgal = dependency('cgal', method:'pkg-config')
freetype = dependency('freetype2', method:'pkg-config')
gtest = dependency('gtest', method:'pkg-config')
rapidyaml = dependency('rapidyaml', method:'pkg-config')
spdlog = dependency('spdlog', method:'pkg-config')
entt = dependency('entt', method:'pkg-config')

if host_machine.system() == 'linux'
  xlib = dependency('x11')
  xkb_common = dependency('xkbcommon')
  wayland_client = dependency('wayland-client')
endif

subdir('shader')

subdir('library/external/mikktspace')

subdir('library/core')
subdir('library/io')

subdir('library/world')
subdir('library/desktop')
subdir('library/desktop_main')
subdir('library/threading')
subdir('library/graphics_api')
subdir('library/font')
subdir('library/geometry')
subdir('library/render_objects')
subdir('library/resource')
subdir('library/render_core')
subdir('library/ui_core')

subdir('library/renderer')

subdir('game/demo')

