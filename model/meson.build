model_files = [
    'cage.obj',
    'house.obj',
    'tree.obj',
    'terrain.obj',
]

model_targets = []

foreach file : model_files
    model_targets += fs.copyfile(file, fs.stem(file) + '.model')
endforeach

models = declare_dependency(sources: model_targets)
