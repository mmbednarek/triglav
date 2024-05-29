# TRIGLAV

Rendering engine based on Vulkan. It uses deferred shading.
It supports a PBR material system, screen space ambient occlusion.

## Supported Platforms

- Linux (X11, Wayland)
- Windows

## Building From Source

- Install conan as described here <https://docs.conan.io/2/installation.html>
- Clone the source code 
- Run `conan install . --build=missing -of buildDir`
- Run `conan build . -of buildDir`

## Running the game

./buildDir/game/demo/demo -buildDir=buildDir -contentDir=game/demo/content

