shader_targets += custom_target(
    input: 'full_screen_vs.slang',
    output: '@BASENAME@.spv',
    command: slang_vs_commands,
)

shader_targets += custom_target('shader_blur_compute_rgba',
    input: 'blur.slang',
    output: '@BASENAME@_rgba.spv',
    depend_files: ['gauss_coefs.slang'],
    command : slang_cs_commands,
)

shader_targets += custom_target('shader_blur_compute_r',
    input: 'blur.slang',
    output: '@BASENAME@_r.spv',
    command : [slang_cs_commands, '-DSINGLE_CHANNEL'],
)
