material_files = [
    'bark.yaml',
    'gold.yaml',
    'grass.yaml',
    'leaves.yaml',
    'pine.yaml',
    'quartz.yaml',
]

material_targets = []

foreach file : material_files
    material_targets += fs.copyfile(file, file)
endforeach

subdir('template')

materials = declare_dependency(sources: material_targets)
