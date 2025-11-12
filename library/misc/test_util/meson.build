test_util_sources = files([
                           'include/triglav/test_util/GTest.hpp',
                           'src/Placeholder.cpp',
                       ])

test_util_lib = static_library('test_util',
                            sources : test_util_sources,
                            dependencies : [gtest],
                            include_directories : ['include/triglav/test_util'],
)

test_util = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : test_util_lib,
    dependencies : [gtest]
)
