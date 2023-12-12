model_files = [
    'house.obj',
    'tree.obj',
]

model_targets = []

foreach file : model_files
    model_targets += fs.copyfile(file, fs.stem(file) + '.model')
endforeach

models = declare_dependency(sources: model_targets)
