font_files = [
    'cantarell.ttf',
    'cantarell_18.yaml',
    'cantarell_20.yaml',
    'cantarell_bold.ttf',
    'cantarell_bold_24.yaml',
    'segoeui.ttf',
    'segoeuib.ttf',
    'segoeui_18.yaml',
    'segoeui_bold_20.yaml',
]

font_targets = []

foreach file : font_files
    font_targets += fs.copyfile(file, file)
endforeach

fonts = declare_dependency(sources: font_targets)
