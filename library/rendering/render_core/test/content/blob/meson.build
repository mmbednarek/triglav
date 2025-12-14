blob_lists = [
    'basic_graphics_expected_bitmap.dat',
    'basic_texture_expected_bitmap.dat',
    'basic_depth_expected_bitmap.dat',
    'multiple_passes_expected_bitmap.dat',
    'depth_target_sample_expected_bitmap.dat',
    'basic_ray_tracing_expected_bitmap.dat',
]

foreach filename : blob_lists
    render_core_test_file_deps += fs.copyfile(filename, filename)
endforeach
