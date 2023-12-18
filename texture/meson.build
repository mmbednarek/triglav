texture_files = [
    'earth.png',
    'house.png',
    'gold.png',
    'grass.png',
    'skybox.png',
    'bark.png',
    'leaves.png',
]

texture_targets = []

foreach file : texture_files
    texture_targets += fs.copyfile(file, file)
endforeach

textures = declare_dependency(sources: texture_targets)
