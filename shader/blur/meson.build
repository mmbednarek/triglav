shader_targets += custom_target('shader_blur_compute',
                                input: 'compute.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_compute_cmds,
)

shader_targets += custom_target('shader_blur_sc_compute',
                                input: 'compute.glsl',
                                output: '@BASENAME@_sc.spv',
                                command: compile_compute_cmds + ['-DSINGLE_CHANNEL'],
)
