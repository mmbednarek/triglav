meta_test_sources = files(
    'Main.cpp',
    'MetaTest.cpp',
)

meta_test_deps = [core, test_util, meta]

meta_test = executable('meta_test',
                     sources : meta_test_sources,
                     dependencies : meta_test_deps,
)

test('Meta Tests', meta_test)
