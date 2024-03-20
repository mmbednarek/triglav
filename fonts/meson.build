font_files = [
    'cantarell.ttf',
    'cantarell_24.yaml',
    'cantarell_bold.ttf',
    'cantarell_bold_24.yaml',
]

font_targets = []

foreach file : font_files
    font_targets += fs.copyfile(file, file)
endforeach

fonts = declare_dependency(sources: font_targets)
