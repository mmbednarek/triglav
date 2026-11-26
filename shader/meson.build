subdir('library/triglav')

shader_build_flags = ['--build', 'release']
if get_option('disable_shader_opt').enabled()
    shader_build_flags = ['--build', 'debug']
endif

shader_library_dir = meson.current_source_dir() + '/library'

slang_cs_commands = [
    shader_compiler,
    '@INPUT@',
    shader_build_flags,
    '-I', shader_library_dir,
    '--entry', 'cs_main',
    '--stage', 'compute',
    '-o', '@OUTPUT@'
]

slang_fs_commands = [
    shader_compiler,
    '@INPUT@',
    shader_build_flags,
    '-I', shader_library_dir,
    '--stage', 'fragment',
    '--entry', 'fs_main',
    '-o', '@OUTPUT@'
]

slang_vs_commands = [
    shader_compiler,
    '@INPUT@',
    shader_build_flags,
    '-I', shader_library_dir,
    '--stage', 'vertex',
    '--entry', 'vs_main',
    '-o', '@OUTPUT@'
]

slang_ds_commands = [
    shader_compiler,
    '@INPUT@',
    shader_build_flags,
    '-I', shader_library_dir,
    '--stage', 'domain',
    '--entry', 'ds_main',
    '-o', '@OUTPUT@'
]

slang_hs_commands = [
    shader_compiler,
    '@INPUT@',
    shader_build_flags,
    '-I', shader_library_dir,
    '--entry', 'hs_main',
    '--stage', 'hull',
    '-o', '@OUTPUT@'
]

shader_targets = []

subdir('bindless_geometry')
subdir('geometry')
subdir('misc')
subdir('particles')
subdir('pass')
subdir('ray_tracing')
subdir('terrain')
subdir('ui')

shaders = declare_dependency(sources : [shader_targets])
