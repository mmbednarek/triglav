shader_targets += custom_target('shader_particles_compute',
                                input: 'update_position.slang',
                                output: '@BASENAME@.cshader',
                                depend_files: [shader_lib_triglav_core, 'particle.slang'],
                                command: slang_cs_commands,
)

shader_targets += custom_target('shader_particles_vertex',
                                input: 'render.slang',
                                output: '@BASENAME@.vshader',
                                depend_files: [shader_lib_triglav_core, 'particle.slang'],
                                command: slang_vs_commands,
)

shader_targets += custom_target('shader_particles_fragment',
                                input: 'render.slang',
                                output: '@BASENAME@.fshader',
                                depend_files: [shader_lib_triglav_core, 'particle.slang'],
                                command: slang_fs_commands,
)
