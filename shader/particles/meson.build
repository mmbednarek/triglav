shader_targets += custom_target('shader_particles_compute',
                                input: 'update_position.slang',
                                output: '@BASENAME@.cshader',
                                command: slang_cs_commands,
)

shader_targets += custom_target('shader_particles_vertex',
                                input: 'render.slang',
                                output: '@BASENAME@.vshader',
                                command: slang_vs_commands,
)

shader_targets += custom_target('shader_particles_fragment',
                                input: 'render.slang',
                                output: '@BASENAME@.fshader',
                                command: slang_fs_commands,
)
