shader_targets += custom_target('text_gemetry_gen_cs',
                                input : 'text_geometry_gen.slang',
                                output : '@BASENAME@.spv',
                                depend_files : ['text_processing.slang'],
                                command : slang_cs_commands,
)

shader_targets += custom_target('text_removal_cs',
                                input : 'text_removal.slang',
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

# RECTANGLE

shader_targets += custom_target('rectangle_insertion',
                                input : 'rectangle_insertion.slang',
                                output : '@BASENAME@_cs.spv',
                                depend_files : ['rectangle_api.slang'],
                                command : slang_cs_commands,
)

shader_targets += custom_target('rectangle_removal',
                                input : 'rectangle_removal.slang',
                                output : '@BASENAME@_cs.spv',
                                depend_files : ['rectangle_api.slang'],
                                command : slang_cs_commands,
)

shader_targets += custom_target('rectangle_render_vs',
                                input : 'rectangle_render.slang',
                                output : '@BASENAME@_vs.spv',
                                depend_files : ['rectangle_api.slang'],
                                command : slang_vs_commands,
)

shader_targets += custom_target('rectangle_render_fs',
                                input : 'rectangle_render.slang',
                                output : '@BASENAME@_fs.spv',
                                depend_files : ['rectangle_api.slang'],
                                command : slang_fs_commands,
)

shader_targets += custom_target('sprite_vs',
                                input : 'sprite.slang',
                                output : '@BASENAME@_vs.spv',
                                command : slang_vs_commands,
)

shader_targets += custom_target('sprite_fs',
                                input : 'sprite.slang',
                                output : '@BASENAME@_fs.spv',
                                command : slang_fs_commands,
)
