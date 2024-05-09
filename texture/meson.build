texture_files = [
    'board.png',
    'earth.png',
    'gold.png',
    'gold_normal.png',
    'grass.png',
    'grass_normal.png',
    'skybox.png',
    'bark.png',
    'bark_normal.png',
    'leaves.png',
    'metal_albedo.jpg',
    'metal_metallic.jpg',
    'metal_normal.jpg',
    'metal_roughness.jpg',
    'noise.png',
    'pine.png',
    'pine_normal.png',
    'quartz.png',
    'quartz_normal.png',
]

texture_targets = []

foreach file : texture_files
    texture_targets += fs.copyfile(file, file)
endforeach

textures = declare_dependency(sources: texture_targets)
