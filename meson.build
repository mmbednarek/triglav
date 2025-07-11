project('triglav_engine',
  ['cpp', 'c'],
  version: '0.1.0',
  license: 'GPL3',
  meson_version: '>= 1.5',
  default_options: [
    'cpp_std=c++23',
    'werror=true',
    'warning_level=3'
  ],
)

message(meson.get_compiler('cpp').get_id())

if meson.get_compiler('cpp').get_id() == 'msvc'
  add_project_arguments('/std:c++latest', language : 'cpp')
  add_project_arguments('/Zc:preprocessor', language : 'cpp')
  add_project_arguments('/wd4201', language : 'cpp')
  add_project_arguments('/wd4324', language : 'cpp')

  if get_option('address_sanitizer').enabled()
    add_project_arguments('/fsanitize=address', language : 'cpp')
  endif
else
  add_project_arguments('-Wno-missing-field-initializers', language : 'cpp')
endif

if host_machine.system() == 'windows' and meson.get_compiler('cpp').get_id() == 'gcc'
  add_project_arguments('-Wa,-mbig-obj', language : 'cpp')
  add_project_arguments('-D_GLIBCXX_USE_CXX11_ABI=1', language : 'cpp')
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

if get_option('disable_debug_utils').enabled()
  add_project_arguments('-DTG_DISABLE_DEBUG_UTILS=1', language: 'cpp')
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
rapidjson = dependency('rapidjson', method:'pkg-config')
ktx = dependency('ktx', method:'pkg-config')

if host_machine.system() == 'linux'
  xlib = dependency('x11')
  xkb_common = dependency('xkbcommon')
  wayland_client = dependency('wayland-client')
endif

subdir('shader')

subdir('library/external/mikktspace')
subdir('library/external/stbi')

subdir('library/core')
subdir('library/io')
subdir('library/memory')

subdir('library/world')
subdir('library/threading')
subdir('library/event')
subdir('library/desktop')
subdir('library/desktop_main')
subdir('library/ktx')
subdir('library/graphics_api')
subdir('library/font')
subdir('library/geometry')
subdir('library/json_util')
subdir('library/render_objects')
subdir('library/gltf')
subdir('library/asset')
subdir('library/resource')
subdir('library/render_core')
subdir('library/ui_core')
subdir('library/renderer')
subdir('library/desktop_ui')

subdir('game/demo')

subdir('tool/triglavcli')

