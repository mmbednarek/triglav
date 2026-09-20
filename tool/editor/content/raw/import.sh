#!/usr/bin/bash

triglav project --set-active triglav_editor
triglav import icons.png -r -o editor/texture/ui_icons.tex
triglav project --set-active demo
