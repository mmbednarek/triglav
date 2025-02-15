shader_targets += custom_target(
    input   : 'ambient_occlusion.slang',
    output  : '@BASENAME@.spv',
    depend_files: shader_lib_common,
    command : slang_fs_commands,
)

shader_targets += custom_target(
    input: 'post_processing.slang',
    output: '@BASENAME@.spv',
    command: slang_fs_commands,
)

shader_targets += custom_target(
    input   : 'shading.slang',
    output  : '@BASENAME@.spv',
    depend_files: shader_lib_common,
    command : [slang_fs_commands, '-DSHOW_SHADOW_CASCADES=0'],
)
