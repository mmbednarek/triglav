slang_compile_cs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@']
slang_compile_vs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-O3', '-o', '@OUTPUT@']
slang_compile_fs = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-O3', '-o', '@OUTPUT@']
slang_compile_rgen = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'rgen_main', '-stage', 'raygeneration', '-O3', '-o', '@OUTPUT@']
slang_compile_rmiss = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'rmiss_main', '-stage', 'miss', '-O3', '-o', '@OUTPUT@']
slang_compile_rchit = ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'rchit_main', '-stage', 'closesthit', '-O3', '-o', '@OUTPUT@']

render_core_test_file_deps += fs.copyfile('index.yaml', 'index.yaml')

subdir('shader/misc')
subdir('shader/testing')
subdir('blob')
subdir('texture')
