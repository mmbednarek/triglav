
slang_compile_cs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@']
slang_compile_vs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-O3', '-o', '@OUTPUT@']
slang_compile_fs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-O3', '-o', '@OUTPUT@']

render_core_test_file_deps += custom_target('test_basic_compute',
                                   input : 'basic_compute.slang',
                                   output : 'basic_compute.spv',
                                   command : slang_compile_cs,
)

render_core_test_file_deps += custom_target('test_basic_graphics_vs',
                                   input : 'basic_graphics.slang',
                                   output : 'basic_graphics_vs.spv',
                                   command : slang_compile_vs,
)

render_core_test_file_deps += custom_target('test_basic_graphics_fs',
                                       input : 'basic_graphics.slang',
                                       output : 'basic_graphics_fs.spv',
                                       command : slang_compile_fs,
)

render_core_test_file_deps += custom_target('test_basic_texture_vs',
                                            input : 'basic_texture.slang',
                                            output : 'basic_texture_vs.spv',
                                            command : slang_compile_vs,
)

render_core_test_file_deps += custom_target('test_basic_texture_fs',
                                            input : 'basic_texture.slang',
                                            output : 'basic_texture_fs.spv',
                                            command : slang_compile_fs,
)

render_core_test_file_deps += custom_target('test_basic_depth_vs',
                                            input : 'basic_depth.slang',
                                            output : 'basic_depth_vs.spv',
                                            command : slang_compile_vs,
)

render_core_test_file_deps += custom_target('test_basic_depth_fs',
                                            input : 'basic_depth.slang',
                                            output : 'basic_depth_fs.spv',
                                            command : slang_compile_fs,
)

render_core_test_file_deps += fs.copyfile('index.yaml', 'index.yaml')
render_core_test_file_deps += fs.copyfile('basic_graphics_expected_bitmap.dat', 'basic_graphics_expected_bitmap.dat')
render_core_test_file_deps += fs.copyfile('basic_texture_expected_bitmap.dat', 'basic_texture_expected_bitmap.dat')
render_core_test_file_deps += fs.copyfile('basic_depth_expected_bitmap.dat', 'basic_depth_expected_bitmap.dat')
render_core_test_file_deps += fs.copyfile('sample.png', 'sample.png')
