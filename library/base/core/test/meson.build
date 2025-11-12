core_test_sources = files(
    'Main.cpp',
    'MathTest.cpp',
    'NameTest.cpp',
    'PoolTest.cpp',
    'StringTest.cpp',
    'UpdateListTest.cpp',
)

core_test_deps = [core, test_util]

core_test = executable('core_test',
                       sources : core_test_sources,
                       dependencies : core_test_deps,
)
test('Core Tests', core_test)
