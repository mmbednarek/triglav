shader_lib_common = files([
        'common/brdf.slang',
        'common/common.slang',
        'common/constants.slang',
        'common/random.slang',
        'common/shadow_map.slang',
        'common/view_properties.slang',
])

slang_compiler = find_program('slangc')

shader_opt_flags = ['-O3']
if get_option('disable_shader_opt').enabled()
    shader_opt_flags = ['-g', '-O0']
endif

slang_cs_commands = [
    slang_compiler,
    '-target' , 'spirv',
    '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency',
    '-entry', 'cs_main',
    '-stage', 'compute',
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
    shader_opt_flags,
    '@INPUT@',
    '-o', '@OUTPUT@'
]

shader_targets = []

subdir('bindless_geometry')
subdir('editor')
subdir('geometry')
subdir('misc')
subdir('particles')
subdir('pass')
subdir('ray_tracing')
subdir('ui')

shaders = declare_dependency(sources: [shader_targets])
