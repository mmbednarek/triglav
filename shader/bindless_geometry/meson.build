shader_lib_material = files([
                                'material/material_core.slang',
                                'material/material.slang',
                                'material/mt_albedo_normal_texture.slang',
                                'material/mt_albedo_texture.slang',
                                'material/mt_all_texture.slang',
                                'material/mt_solid_color.slang',
                                'material/texture_bindings.slang',
                            ])

shader_lib_mesh = files(['mesh/mesh.slang'])



shader_targets += custom_target('shader_bindless_geometry_vertex',
                                input : 'bindless_geometry.slang',
                                output : 'vertex.spv',
                                depend_files : [shader_lib_common, shader_lib_material, shader_lib_mesh],
                                command : slang_vs_commands,
)

shader_targets += custom_target('shader_bindless_geometry_fragment',
                                input : 'bindless_geometry.slang',
                                output : 'fragment.spv',
                                depend_files : [shader_lib_common, shader_lib_material, shader_lib_mesh],
                                command : slang_fs_commands,
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