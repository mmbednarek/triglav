threading_test_sources = files(
    'ThreadPoolTest.cpp',
    'Main.cpp',
)

threading_test_deps = [threading, testing_core]

threading_test = executable('threading_test',
                       sources : threading_test_sources,
                       dependencies : threading_test_deps,
)

test('Threading Tests', threading_test)
