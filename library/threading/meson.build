threading_sources = files([
  'include/triglav/threading/DoubleBufferQueue.hpp',
  'include/triglav/threading/SafeAccess.hpp',
  'include/triglav/threading/SharedMutex.hpp',
  'include/triglav/threading/Threading.hpp',
  'include/triglav/threading/ThreadPool.hpp',
  'src/ThreadPool.cpp',
  'src/Threading.cpp',
])

if host_machine.system() == 'linux'
    subdir('platform/linux')
elif host_machine.system() == 'windows'
    subdir('platform/windows')
endif

threading_deps = [core, spdlog]
threading_incl = include_directories(['include', 'include/triglav/threading'])

threading_lib = static_library('threading',
                              sources : threading_sources,
                              dependencies : threading_deps,
                              include_directories : threading_incl,
)

threading = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : threading_lib,
    dependencies : threading_deps,
)

subdir('test')
