editor_shader_targets = []

editor_shader_targets += custom_target(
    input   : 'object_selection.slang',
    output  : '@BASENAME@.vshader',
    depend_files: shader_lib_triglav_core,
    command : slang_vs_commands,
)

editor_shader_targets += custom_target(
    input   : 'object_selection.slang',
    output  : '@BASENAME@.fshader',
    depend_files: shader_lib_triglav_core,
    command : slang_fs_commands,
)

editor_shaders = declare_dependency(sources: [editor_shader_targets])
