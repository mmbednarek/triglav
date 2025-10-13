memory_sources = files([
                              'include/triglav/memory/HeapAllocator.hpp',
                              'include/triglav/memory/Memory.hpp',
                              'src/HeapAllocator.cpp',
                              'src/Memory.cpp',
                          ])

memory_deps = [core]
memory_incl = include_directories(['include', 'include/triglav/memory'])

memory_lib = static_library('memory',
                               sources: memory_sources,
                               dependencies: memory_deps,
                               include_directories: memory_incl,
)

memory = declare_dependency(
    include_directories: include_directories(['include']),
    link_with: [memory_lib],
    dependencies: memory_deps,
)

subdir('test')