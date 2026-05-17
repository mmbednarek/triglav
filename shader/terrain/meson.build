shader_targets += custom_target(
    input: 'vertex.slang',
    output: '@BASENAME@.vshader',
    command: slang_vs_commands,
)

shader_targets += custom_target(
    input : 'fragment.slang',
    output : '@BASENAME@.fshader',
    depend_files: shader_lib_triglav_core,
    command : [slang_fs_commands, '-DMT_USE_BONES=0', '-DMT_USE_NORMAL_MAP=0', '-DMT_USE_ROUGHNESS_MAP=0', '-DMT_USE_METALIC_MAP=0'],
)

shader_targets += custom_target(
    input   : 'domain.slang',
    output  : '@BASENAME@.dshader',
    depend_files: shader_lib_triglav_core,
    command : slang_ds_commands,
)

shader_targets += custom_target(
    input   : 'hull.slang',
    output  : '@BASENAME@.hshader',
    depend_files: shader_lib_triglav_core,
    command : slang_hs_commands,
)
