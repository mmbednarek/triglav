shader_targets += custom_target(
    input   : 'object_selection.slang',
    output  : '@BASENAME@_vs.spv',
    depend_files: shader_lib_common,
    command : slang_vs_commands,
)

shader_targets += custom_target(
    input   : 'object_selection.slang',
    output  : '@BASENAME@_fs.spv',
    depend_files: shader_lib_common,
    command : slang_fs_commands,
)
