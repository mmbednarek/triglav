threading_sources = files([
  'include/triglav/threading/DoubleBufferQueue.hpp',
  'include/triglav/threading/SafeAccess.hpp',
  'include/triglav/threading/Threading.h',
  'include/triglav/threading/ThreadPool.h',
  'src/ThreadPool.cpp',
  'src/Threading.cpp',
])

threading_deps = [core]
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
