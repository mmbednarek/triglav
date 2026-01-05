meta_sources = files([
                          'include/triglav/meta/Display.hpp',
                          'include/triglav/meta/Meta.hpp',
                          'include/triglav/meta/TypeRegistry.hpp',
                          'src/Box.cpp',
                          'src/Display.cpp',
                          'src/ObjectRef.cpp',
                          'src/TypeRegisterer.cpp',
                          'src/TypeRegistry.cpp',
                      ])

meta_deps = [core, io]
meta_incl = include_directories(['include', 'include/triglav/meta'])

meta_lib = static_library('meta',
                           sources : meta_sources,
                           dependencies : meta_deps,
                           include_directories : meta_incl,
)

meta = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : meta_lib,
    dependencies : meta_deps,
)

subdir('test')