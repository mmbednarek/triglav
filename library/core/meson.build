core_sources = files([
                    'include/triglav/Name.hpp',
                    'include/triglav/ResourceType.hpp',
                    'include/triglav/detail/Crc.hpp',
])

core_deps = [glm]
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