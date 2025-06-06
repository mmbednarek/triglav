shader_lib_material = files([
                                'material/material_core.slang',
                                'material/material.slang',
                                'material/texture_bindings.slang',
                            ])

shader_lib_mesh = files(['mesh/mesh.slang'])



shader_targets += custom_target('shader_bindless_geometry_vertex',
                                input : 'bindless_geometry.slang',
                                output : 'vertex.spv',
                                depend_files : [shader_lib_common, shader_lib_material, shader_lib_mesh],
                                command : slang_vs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_culling_compute',
                                input : 'culling.slang',
                                output : 'culling_comp.spv',
                                depend_files : [shader_lib_common, shader_lib_mesh],
                                command : slang_cs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_depth_prepass_vertex',
                                input : 'depth_prepass.slang',
                                output : 'depth_prepass_vertex.spv',
                                depend_files : [shader_lib_common, shader_lib_mesh],
                                command : slang_vs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_depth_prepass_fragment',
                                input : 'depth_prepass.slang',
                                output : 'depth_prepass_fragment.spv',
                                depend_files : [shader_lib_common, shader_lib_mesh],
                                command : slang_fs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_shadow_map_vertex',
                                input : 'shadow_map.slang',
                                output : 'shadow_map_vertex.spv',
                                depend_files : [shader_lib_common, shader_lib_mesh],
                                command : slang_vs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_shadow_map_fragment',
                                input : 'shadow_map.slang',
                                output : 'shadow_map_fragment.spv',
                                depend_files : [shader_lib_common, shader_lib_mesh],
                                command : slang_fs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_hi_z_compute',
                                input : 'hi_zbuffer_construct.slang',
                                output : 'hi_z_comp.spv',
                                command : slang_cs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_render_mt0',
                                input : 'render_geometry.slang',
                                output : 'render_mt0.spv',
                                depend_files : [shader_lib_common, shader_lib_material, shader_lib_mesh],
                                command : [slang_fs_commands, '-DMT_USE_NORMAL_MAP=0', '-DMT_USE_ROUGHNESS_MAP=0', '-DMT_USE_METALIC_MAP=0'],
)

shader_targets += custom_target('shader_bindless_geometry_render_mt1',
                                input : 'render_geometry.slang',
                                output : 'render_mt1.spv',
                                depend_files : [shader_lib_common, shader_lib_material, shader_lib_mesh],
                                command : [slang_fs_commands, '-DMT_USE_NORMAL_MAP=1', '-DMT_USE_ROUGHNESS_MAP=0', '-DMT_USE_METALIC_MAP=0'],
)

shader_targets += custom_target('shader_bindless_geometry_render_mt2',
                                input : 'render_geometry.slang',
                                output : 'render_mt2.spv',
                                depend_files : [shader_lib_common, shader_lib_material, shader_lib_mesh],
                                command : [slang_fs_commands, '-DMT_USE_NORMAL_MAP=1', '-DMT_USE_ROUGHNESS_MAP=1', '-DMT_USE_METALIC_MAP=1'],
)
