shader_targets += custom_target('shader_post_processing_vertex',
                                input : 'vertex.glsl',
                                output : '@BASENAME@.spv',
                                command : compile_vertex_cmds,
)

# shader_targets += fs.copyfile('vertex.spv', 'vertex.spv')

shader_targets += custom_target('shader_post_processing_fragment',
                                input : 'fragment.glsl',
                                output : '@BASENAME@.spv',
                                command : compile_fragment_cmds,
)
