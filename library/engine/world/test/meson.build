world_test_sources = files(
    'ComponentTest.cpp',
    'Main.cpp'
)

world_test_deps = [world, testing_core, io]

world_test = executable('world_test',
                           sources: world_test_sources,
                           dependencies: world_test_deps,
)

test('World Tests', world_test)