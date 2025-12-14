shader_targets += custom_target(
    input: 'full_screen.slang',
    output: '@BASENAME@.vshader',
    command: slang_vs_commands,
)

shader_targets += custom_target('shader_blur_compute_rgba',
    input: 'blur.slang',
    output: '@BASENAME@_rgba.cshader',
    depend_files: ['gauss_coefs.slang'],
    command : slang_cs_commands,
)

shader_targets += custom_target('shader_blur_compute_r',
    input: 'blur.slang',
    output: '@BASENAME@_r.cshader',
    command : [slang_cs_commands, '-DSINGLE_CHANNEL'],
)
