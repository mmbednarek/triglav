texture_files = [
    'earth.png',
    'gold.png',
    'grass.png',
    'grass_normal.png',
    'skybox.png',
    'bark.png',
    'bark_normal.png',
    'leaves.png',
    'pine.png',
    'pine_normal.png',
]

texture_targets = []

foreach file : texture_files
    texture_targets += fs.copyfile(file, file)
endforeach

textures = declare_dependency(sources: texture_targets)
