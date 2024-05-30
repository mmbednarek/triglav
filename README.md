# TRIGLAV

Rendering engine based on Vulkan. It uses deferred shading.
It supports a PBR material system and screen space ambient occlusion.

## Supported Platforms

- Linux (X11, Wayland)
- Windows

## Building From Source

- Install Conan as described here <https://docs.conan.io/2/installation.html>
- Install Meson as descibed here <https://mesonbuild.com/Quick-guide.html>.
- Clone the source code.
- Run `conan install . --build=missing -of buildDir`.
- Run `conan build . -of buildDir`.

## Running the demo

Once the demo is built you can start it with the following command:

```
./buildDir/game/demo/demo -buildDir=buildDir -contentDir=game/demo/content
```

## Movement

Move around - WSAD.

Jump - Space.

Hold middle mouse button to look around.

