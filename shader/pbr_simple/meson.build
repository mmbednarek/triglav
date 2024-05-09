shader_targets += custom_target('shader_pbr_simple_vertex',
                                input: 'vertex.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_vertex_cmds,
)

shader_targets += custom_target('shader_pbr_simple_fragment',
                                input: 'fragment.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_fragment_cmds,
)
