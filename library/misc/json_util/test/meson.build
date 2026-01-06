json_util_test_sources = files(
    'DeserializationTest.cpp',
    'Main.cpp',
)

json_util_test_deps = [core, test_util, json_util]

json_util_test = executable('json_util_test',
                       sources : json_util_test_sources ,
                       dependencies : json_util_test_deps,
)

test('Json Util Tests', json_util_test)
