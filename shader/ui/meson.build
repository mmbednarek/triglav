shader_targets += custom_target('text_gemetry_gen_cs',
                                input : 'text_geometry_gen.slang',
                                output : '@BASENAME@.spv',
                                depend_files : ['text_processing.slang'],
                                command : slang_cs_commands,
)

shader_targets += custom_target('text_render_vs',
                                input : 'text_render.slang',
                                output : '@BASENAME@_vs.spv',
                                depend_files : ['text_processing.slang'],
                                command : slang_vs_commands,
)

shader_targets += custom_target('text_render_fs',
                                input : 'text_render.slang',
                                output : '@BASENAME@_fs.spv',
                                depend_files : ['text_processing.slang'],
                                command : slang_fs_commands,
)

shader_targets += custom_target('rectangle_vs',
                                input : 'rectangle.slang',
                                output : '@BASENAME@_vs.spv',
                                command : slang_vs_commands,
)

shader_targets += custom_target('rectangle_fs',
                                input : 'rectangle.slang',
                                output : '@BASENAME@_fs.spv',
                                command : slang_fs_commands,
)
