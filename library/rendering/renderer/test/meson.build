renderer_test_sources = files(
    'DrawCallTest.cpp',
    'Main.cpp',
)

renderer_test_file_deps = []

renderer_test_deps = [renderer,
                         testing_core,
                         testing_render_util,
                         shaders,
                         desktop,
]

renderer_test = executable('renderer_test',
                              sources : renderer_test_sources,
                              dependencies : renderer_test_deps,
                              link_depends: renderer_test_file_deps,
                              win_subsystem: 'windows',
                              link_whole: [desktop_main_lib],
)

test('Renderer Test', renderer_test, workdir: meson.current_build_dir())
