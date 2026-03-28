shader_targets += custom_target('shader_bindless_geometry_animation_compute',
                                input : 'animation.slang',
                                output : 'animation.cshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_mesh],
                                command : [slang_cs_commands],
)

shader_targets += custom_target('shader_bindless_geometry_vertex_vl0',
                                input : 'bindless_geometry.slang',
                                output : 'vertex_vl0.vshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_material, shader_lib_triglav_mesh],
                                command : [slang_vs_commands, '-DMT_USE_NORMAL_MAP=0'],
)

shader_targets += custom_target('shader_bindless_geometry_vertex_vl1',
                                input : 'bindless_geometry.slang',
                                output : 'vertex_vl1.vshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_material, shader_lib_triglav_mesh],
                                command : [slang_vs_commands, '-DMT_USE_NORMAL_MAP=1'],
)

shader_targets += custom_target('shader_bindless_geometry_culling_compute',
                                input : 'culling.slang',
                                output : 'culling.cshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_mesh],
                                command : slang_cs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_depth_prepass_vertex',
                                input : 'depth_prepass.slang',
                                output : 'depth_prepass.vshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_mesh],
                                command : slang_vs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_depth_prepass_fragment',
                                input : 'depth_prepass.slang',
                                output : 'depth_prepass.fshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_mesh],
                                command : slang_fs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_shadow_map_vertex',
                                input : 'shadow_map.slang',
                                output : 'shadow_map.vshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_mesh],
                                command : slang_vs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_shadow_map_fragment',
                                input : 'shadow_map.slang',
                                output : 'shadow_map.fshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_mesh],
                                command : slang_fs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_hi_z_compute',
                                input : 'hi_zbuffer_construct.slang',
                                output : 'hi_zbuffer_construct.cshader',
                                command : slang_cs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_passthrough',
                                input : 'passthrough.slang',
                                output : 'passthrough.cshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_mesh],
                                command : slang_cs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_render_mt0',
                                input : 'render_geometry.slang',
                                output : 'render_mt0.fshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_material, shader_lib_triglav_mesh],
                                command : [slang_fs_commands, '-DMT_USE_NORMAL_MAP=0', '-DMT_USE_ROUGHNESS_MAP=0', '-DMT_USE_METALIC_MAP=0'],
)

shader_targets += custom_target('shader_bindless_geometry_render_mt1',
                                input : 'render_geometry.slang',
                                output : 'render_mt1.fshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_material, shader_lib_triglav_mesh],
                                command : [slang_fs_commands, '-DMT_USE_NORMAL_MAP=1', '-DMT_USE_ROUGHNESS_MAP=0', '-DMT_USE_METALIC_MAP=0'],
)

shader_targets += custom_target('shader_bindless_geometry_render_mt2',
                                input : 'render_geometry.slang',
                                output : 'render_mt2.fshader',
                                depend_files : [shader_lib_triglav_core, shader_lib_triglav_material, shader_lib_triglav_mesh],
                                command : [slang_fs_commands, '-DMT_USE_NORMAL_MAP=1', '-DMT_USE_ROUGHNESS_MAP=1', '-DMT_USE_METALIC_MAP=1'],
)

shader_targets += custom_target('shader_bindless_geometry_transform_to_matrix',
                                input : 'transform_to_matrix.slang',
                                output : 'transform_to_matrix.cshader',
                                depend_files : [shader_lib_triglav_core],
                                command : slang_cs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_matrix_multiply',
                                input : 'matrix_multiply.slang',
                                output : 'matrix_multiply.cshader',
                                depend_files : [shader_lib_triglav_core],
                                command : slang_cs_commands,
)
