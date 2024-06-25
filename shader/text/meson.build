shader_targets += custom_target('shader_text_compute',
                                input: 'compute.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_compute_cmds,
)

shader_targets += custom_target('shader_text_vertex',
                                input: 'vertex.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_vertex_cmds,
)

shader_targets += custom_target('shader_text_fragment',
                                input: 'fragment.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_fragment_cmds,
)
