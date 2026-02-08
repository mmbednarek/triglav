subdir('library/triglav')

slang_compiler = find_program('slangc')

shader_opt_flags = ['-O3']
if get_option('disable_shader_opt').enabled()
    shader_opt_flags = ['-g', '-O0']
endif

shader_library_dir = meson.current_source_dir() + '/library'

slang_cs_commands = [
    slang_compiler,
    '-target' , 'spirv',
    '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency',
    '-entry', 'cs_main',
    '-stage', 'compute',
    '-I', shader_library_dir,
    shader_opt_flags,
    '@INPUT@',
    '-o', '@OUTPUT@'
]

slang_fs_commands = [
    slang_compiler,
    '-target' , 'spirv',
    '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency',
    '-entry', 'fs_main',
    '-stage', 'fragment',
    '-I', shader_library_dir,
    shader_opt_flags,
    '@INPUT@',
    '-o', '@OUTPUT@'
]

slang_vs_commands = [
    slang_compiler,
    '-target' , 'spirv',
    '-profile', 'spirv_1_6',
    '-entry', 'vs_main',
    '-stage', 'vertex',
    '-I', shader_library_dir,
    shader_opt_flags,
    '@INPUT@',
    '-o', '@OUTPUT@'
]

shader_targets = []

subdir('bindless_geometry')
subdir('geometry')
subdir('misc')
subdir('particles')
subdir('pass')
subdir('ray_tracing')
subdir('ui')

shaders = declare_dependency(sources: [shader_targets])
