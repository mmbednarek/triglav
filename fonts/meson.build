font_files = [
    'cantarell.ttf',
    'cantarell_bold.ttf',
]

font_targets = []

foreach file : font_files
    font_targets += fs.copyfile(file, file)
endforeach

fonts = declare_dependency(sources: font_targets)
