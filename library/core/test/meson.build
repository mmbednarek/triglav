core_test_sources = files(
    'Main.cpp',
    'MathTest.cpp',
    'NameTest.cpp',
    'PoolTest.cpp',
    'StringTest.cpp',
)

core_test_deps = [core, gtest]

core_test = executable('core_test',
                       sources : core_test_sources,
                       dependencies : core_test_deps,
)
test('Core Tests', core_test)
