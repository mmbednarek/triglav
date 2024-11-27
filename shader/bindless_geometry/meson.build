shader_targets += custom_target('shader_bindless_geometry_vertex',
                                input : 'bindless_geometry.slang',
                                output : 'vertex.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_bindless_geometry_fragment',
                                input : 'bindless_geometry.slang',
                                output : 'fragment.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_bindless_geometry_culling_compute',
                                input : 'culling.slang',
                                output : 'culling_comp.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_bindless_geometry_depth_prepass_vertex',
                                input : 'depth_prepass.slang',
                                output : 'depth_prepass_vertex.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_bindless_geometry_depth_prepass_fragment',
                                input : 'depth_prepass.slang',
                                output : 'depth_prepass_fragment.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_bindless_geometry_hi_z_compute',
                                input : 'hi_zbuffer_construct.slang',
                                output : 'hi_z_comp.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@'],
)