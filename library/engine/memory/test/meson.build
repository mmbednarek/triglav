memory_test_sources = files(
    'HeapAllocatorTest.cpp',
    'Main.cpp',
)

memory_test_deps = [memory, test_util]

memory_test = executable('memory_test',
                            sources : memory_test_sources,
                            dependencies : memory_test_deps,
)

test('Memory Tests', memory_test)
