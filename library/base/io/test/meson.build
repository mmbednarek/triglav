io_test_sources = files(
    'DynamicWriterTest.cpp',
    'Main.cpp',
)

io_test_deps = [core, testing_core, io]

io_test = executable('io_test',
                     sources : io_test_sources,
                     dependencies : io_test_deps,
)

test('IO Tests', io_test)
