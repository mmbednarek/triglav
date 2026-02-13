testing_core_sources = files([
                           'include/triglav/testing_core/GTest.hpp',
                           'src/Placeholder.cpp',
                       ])

testing_core_lib = static_library('testing_core',
                            sources : testing_core_sources,
                            dependencies : [gtest],
                            include_directories : ['include/triglav/testing_core'],
)

testing_core = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : testing_core_lib,
    dependencies : [gtest]
)
