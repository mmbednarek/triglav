compute_shaders = [
    'basic_compute.slang',
    'increase_number.slang',
]

vertex_shaders = [
    'basic_graphics.slang',
    'basic_texture.slang',
    'basic_depth.slang',
    'full_screen.slang',
    'depth_target_sample_draw.slang',
]

fragment_shaders = [
    'basic_graphics.slang',
    'basic_texture.slang',
    'basic_depth.slang',
    'multiple_passes_first.slang',
    'multiple_passes_second.slang',
    'depth_target_sample_draw.slang',
    'depth_target_sample_sample.slang',
]

raygen_shaders = [
    'basic_ray_tracing.slang'
]

raymiss_shaders = [
    'basic_ray_tracing.slang'
]

rayclosesthit_shaders = [
    'basic_ray_tracing.slang'
]

files_to_copy = [
    'index.yaml',
    'basic_graphics_expected_bitmap.dat',
    'basic_texture_expected_bitmap.dat',
    'basic_depth_expected_bitmap.dat',
    'multiple_passes_expected_bitmap.dat',
    'depth_target_sample_expected_bitmap.dat',
    'basic_ray_tracing_expected_bitmap.dat',
    'sample.png',
]

slang_compile_cs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@']

foreach shader : compute_shaders
    render_core_test_file_deps += custom_target(input   : shader,
                                                output  : '@BASENAME@.spv',
                                                command : slang_compile_cs,
    )
endforeach

slang_compile_vs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-O3', '-o', '@OUTPUT@']

foreach shader : vertex_shaders
    render_core_test_file_deps += custom_target(input   : shader,
                                                output  : '@BASENAME@_vs.spv',
                                                command : slang_compile_vs,
    )
endforeach

slang_compile_fs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-O3', '-o', '@OUTPUT@']

foreach shader : fragment_shaders
    render_core_test_file_deps += custom_target(input   : shader,
                                                output  : '@BASENAME@_fs.spv',
                                                command : slang_compile_fs,
    )
endforeach

slang_compile_rgen = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'rgen_main', '-stage', 'raygeneration', '-O3', '-o', '@OUTPUT@']

foreach shader : raygen_shaders
    render_core_test_file_deps += custom_target(input   : shader,
                                                output  : '@BASENAME@_rgen.spv',
                                                command : slang_compile_rgen,
    )
endforeach

slang_compile_rmiss = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'rmiss_main', '-stage', 'miss', '-O3', '-o', '@OUTPUT@']

foreach shader : raymiss_shaders
    render_core_test_file_deps += custom_target(input   : shader,
                                                output  : '@BASENAME@_rmiss.spv',
                                                command : slang_compile_rmiss ,
    )
endforeach

slang_compile_rchit = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'rchit_main', '-stage', 'closesthit', '-O3', '-o', '@OUTPUT@']

foreach shader : rayclosesthit_shaders
    render_core_test_file_deps += custom_target(input   : shader,
                                                output  : '@BASENAME@_rchit.spv',
                                                command : slang_compile_rchit,
    )
endforeach

foreach filename : files_to_copy
    render_core_test_file_deps += fs.copyfile(filename, filename)
endforeach
