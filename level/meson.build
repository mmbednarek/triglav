level_files = [
    'demo.yaml',
]

level_targets = []

foreach file : level_files
    level_targets += fs.copyfile(file, file)
endforeach

levels = declare_dependency(sources: level_targets)
