resource_test_sources = files(
    'Main.cpp',
    'ParserTest.cpp'
)

resource_test_deps = [resource, test_util, io]

resource_test = executable('resource_test',
                           sources: resource_test_sources,
                           dependencies: resource_test_deps,
)

# TODO: Fix resource tests
# test('Resource Tests', resource_test)