material_files = [
    'bark.yaml',
    'brick.yaml',
    'gold.yaml',
    'grass.yaml',
    'leaves.yaml',
    'metal.yaml',
    'pine.yaml',
    'quartz.yaml',
    'stone.yaml',
]

material_targets = []

foreach file : material_files
    material_targets += fs.copyfile(file, file)
endforeach

subdir('template')

materials = declare_dependency(sources: material_targets)
