model_files = [
    'cage.obj',
    'tree.obj',
    'terrain.obj',
    'teapot.obj',
    'bunny.obj',
    'pine.obj',
]

model_targets = []

foreach file : model_files
    model_targets += fs.copyfile(file, fs.stem(file) + '.model')
endforeach

models = declare_dependency(sources: model_targets)
