geometry_test_sources = files(
    'BVHTest.cpp',
    'Main.cpp',
)

geometry_test_file_deps = []

geometry_test_deps = [geometry, test_util]

geometry_test = executable('geometry_test',
                              sources : geometry_test_sources,
                              dependencies : geometry_test_deps,
                              link_depends: geometry_test_file_deps,
)

test('Geometry Test', geometry_test, workdir: meson.current_build_dir())
