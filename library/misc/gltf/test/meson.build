gltf_test_sources = files(
    'DeserializationTest.cpp',
    'Main.cpp',
    'TestData.hpp',
)


gltf_test_data_file = fs.copyfile('BoxTextured0.bin', 'BoxTextured0.bin')
gltf_test_example_glb = fs.copyfile('example.glb', 'example.glb')

gltf_test_deps = [core, test_util, gltf]

gltf_test = executable('gltf_test',
                     sources : [gltf_test_sources, gltf_test_data_file, gltf_test_example_glb],
                     dependencies : gltf_test_deps,
)

test('GLTF Tests', gltf_test)