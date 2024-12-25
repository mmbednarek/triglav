
render_core_test_file_deps += custom_target('test_basic_compute',
                                   input : 'basic_compute.slang',
                                   output : 'basic_compute.spv',
                                   command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@'],
)

render_core_test_file_deps += custom_target('test_basic_graphics_vs',
                                   input : 'basic_graphics.slang',
                                   output : 'basic_graphics_vs.spv',
                                   command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-O3', '-o', '@OUTPUT@'],
)

render_core_test_file_deps += custom_target('test_basic_graphics_fs',
                                       input : 'basic_graphics.slang',
                                       output : 'basic_graphics_fs.spv',
                                       command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-O3', '-o', '@OUTPUT@'],
)

render_core_test_file_deps += fs.copyfile('index.yaml', 'index.yaml')
render_core_test_file_deps += fs.copyfile('basic_graphics_expected_bitmap.dat', 'basic_graphics_expected_bitmap.dat')
