shader_targets += custom_target('shader_brdf_normal_map_vertex',
                                input: 'vertex.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_vertex_cmds,
)

shader_targets += custom_target('shader_brdf_normal_map_fragment',
                                input: 'fragment.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_fragment_cmds,
)
