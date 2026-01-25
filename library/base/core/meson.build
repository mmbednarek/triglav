core_sources = files([
                    'include/triglav/detail/Crc.hpp',
                    'include/triglav/Array.hpp',
                    'include/triglav/ArrayMap.hpp',
                    'include/triglav/BuildInfo.hpp',
                    'include/triglav/CompTimeString.hpp',
                    'include/triglav/Debug.hpp',
                    'include/triglav/EnumFlags.hpp',
                    'include/triglav/Format.hpp',
                    'include/triglav/Int.hpp',
                    'include/triglav/Logging.hpp',
                    'include/triglav/Macros.hpp',
                    'include/triglav/Math.hpp',
                    'include/triglav/Name.hpp',
                    'include/triglav/NameResolution.hpp',
                    'include/triglav/ObjectPool.hpp',
                    'include/triglav/RangedArray.hpp',
                    'include/triglav/Ranges.hpp',
                    'include/triglav/ResourcePathMap.hpp',
                    'include/triglav/ResourceType.hpp',
                    'include/triglav/String.hpp',
                    'include/triglav/Template.hpp',
                    'include/triglav/TypeMacroList.hpp',
                    'include/triglav/UpdateList.hpp',
                    'include/triglav/Utf8.hpp',
                    'src/Core.cpp',
                    'src/Logging.cpp',
                    'src/Math.cpp',
                    'src/NameResolution.cpp',
                    'src/ResourcePathMap.cpp',
                    'src/String.cpp'
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