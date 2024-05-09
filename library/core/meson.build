core_sources = files([
                    'include/triglav/detail/Crc.hpp',
                    'include/triglav/Array.hpp',
                    'include/triglav/CompTimeString.hpp',
                    'include/triglav/Delegate.hpp',
                    'include/triglav/Entt.hpp',
                    'include/triglav/EnumFlags.hpp',
                    'include/triglav/Heap.hpp',
                    'include/triglav/Int.hpp',
                    'include/triglav/Name.hpp',
                    'include/triglav/ObjectPool.hpp',
                    'include/triglav/ResourceType.hpp',
                    'include/triglav/TypeMacroList.hpp',
                    'src/Core.cpp'
])

core_deps = [glm, entt]
core_incl = include_directories(['include', 'include/triglav'])

core_lib = static_library('triglav_core',
  sources: core_sources,
  dependencies: core_deps,
  include_directories: core_incl,
)

core = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: core_lib,
  dependencies: core_deps,
)

subdir('test')