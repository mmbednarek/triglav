shader_targets += custom_target(
    input   : 'object_selection.slang',
    output  : '@BASENAME@.vshader',
    depend_files: shader_lib_common,
    command : slang_vs_commands,
)

shader_targets += custom_target(
    input   : 'object_selection.slang',
    output  : '@BASENAME@.fshader',
    depend_files: shader_lib_common,
    command : slang_fs_commands,
)
