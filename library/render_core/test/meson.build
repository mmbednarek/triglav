render_core_test_sources = files(
    'BuildContextTest.cpp',
    'TestingSupport.cpp',
    'TestingSupport.hpp',
    'Main.cpp',
)

render_core_test_file_deps = []
subdir('content')

render_core_test_deps = [render_core,
                         gtest,
                         desktop,
]

render_core_test = executable('render_core_test',
                       sources : render_core_test_sources,
                       dependencies : render_core_test_deps,
                       link_depends: render_core_test_file_deps,
                       win_subsystem: 'windows',
                       link_whole: [desktop_main_lib],
)
