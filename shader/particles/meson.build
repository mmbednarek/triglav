shader_targets += custom_target('shader_particles_compute',
                                input: 'update_position.slang',
                                output: 'compute.spv',
                                command: slang_cs_commands,
)

shader_targets += custom_target('shader_particles_vertex',
                                input: 'render.slang',
                                output: 'vertex.spv',
                                command: slang_vs_commands,
)

shader_targets += custom_target('shader_particles_fragment',
                                input: 'render.slang',
                                output: 'fragment.spv',
                                command: slang_fs_commands,
)
