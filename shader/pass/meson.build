shader_targets += custom_target(
    input   : 'ambient_occlusion.slang',
    output  : '@BASENAME@.fshader',
    depend_files: shader_lib_triglav_core,
    command : slang_fs_commands,
)

shader_targets += custom_target(
    input: 'post_processing.slang',
    output: '@BASENAME@.fshader',
    command: slang_fs_commands,
)

shader_targets += custom_target(
    input   : 'shading.slang',
    output  : '@BASENAME@.fshader',
    depend_files: shader_lib_triglav_core,
    command : [slang_fs_commands, '-DSHOW_SHADOW_CASCADES=0'],
)
