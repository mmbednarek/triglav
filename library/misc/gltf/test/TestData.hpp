#pragma once

namespace triglav::test {

constexpr auto g_gltf_example = R"(
{
    "asset": {
        "generator": "COLLADA2GLTF",
        "version": "2.0"
    },
    "scene": 0,
    "scenes": [
        {
            "nodes": [
                0
            ]
        }
    ],
    "nodes": [
        {
            "children": [
                1
            ],
            "matrix": [
                1.0,
                0.0,
                0.0,
                0.0,
                0.0,
                0.0,
                -1.0,
                0.0,
                0.0,
                1.0,
                0.0,
                0.0,
                0.0,
                0.0,
                0.0,
                1.0
            ]
        },
        {
            "mesh": 0
        }
    ],
    "meshes": [
        {
            "primitives": [
                {
                    "attributes": {
                        "NORMAL": 1,
                        "POSITION": 2,
                        "TEXCOORD_0": 3
                    },
                    "indices": 0,
                    "mode": 4,
                    "material": 0
                }
            ],
            "name": "Mesh"
        }
    ],
    "accessors": [
        {
            "buffer_view": 0,
            "byte_offset": 0,
            "component_type": 5123,
            "count": 36,
            "max": [
                23
            ],
            "min": [
                0
            ],
            "type": "SCALAR"
        },
        {
            "buffer_view": 1,
            "byte_offset": 0,
            "component_type": 5126,
            "count": 24,
            "max": [
                1.0,
                1.0,
                1.0
            ],
            "min": [
                -1.0,
                -1.0,
                -1.0
            ],
            "type": "VEC3"
        },
        {
            "buffer_view": 1,
            "byte_offset": 288,
            "component_type": 5126,
            "count": 24,
            "max": [
                0.5,
                0.5,
                0.5
            ],
            "min": [
                -0.5,
                -0.5,
                -0.5
            ],
            "type": "VEC3"
        },
        {
            "buffer_view": 2,
            "byte_offset": 0,
            "component_type": 5126,
            "count": 24,
            "max": [
                6.0,
                1.0
            ],
            "min": [
                0.0,
                0.0
            ],
            "type": "VEC2"
        }
    ],
    "materials": [
        {
            "pbr_metallic_roughness": {
                "base_color_texture": {
                    "index": 0
                },
                "metallic_factor": 0.0
            },
            "name": "Texture"
        }
    ],
    "textures": [
        {
            "sampler": 0,
            "source": 0
        }
    ],
    "images": [
        {
            "uri": "CesiumLogoFlat.png"
        }
    ],
    "samplers": [
        {
            "mag_filter": 9729,
            "min_filter": 9986,
            "wrap_s": 10497,
            "wrap_t": 10497
        }
    ],
    "buffer_views": [
        {
            "buffer": 0,
            "byte_offset": 768,
            "byte_length": 72,
            "target": 34963
        },
        {
            "buffer": 0,
            "byte_offset": 0,
            "byte_length": 576,
            "byte_stride": 12,
            "target": 34962
        },
        {
            "buffer": 0,
            "byte_offset": 576,
            "byte_length": 192,
            "byte_stride": 8,
            "target": 34962
        }
    ],
    "buffers": [
        {
            "byte_length": 840,
            "uri": "BoxTextured0.bin"
        }
    ]
}
)";

}