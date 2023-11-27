.PHONY: all shaders clean example_shader skybox_shader

all: shaders

shaders: example_shader skybox_shader

example_shader: shader/example_fragment.spv shader/example_vertex.spv

shader/example_fragment.spv: shader/example_fragment.glsl
	glslc -fshader-stage=frag shader/example_fragment.glsl -o shader/example_fragment.spv

shader/example_vertex.spv: shader/example_vertex.glsl
	glslc -fshader-stage=vert shader/example_vertex.glsl -o shader/example_vertex.spv

skybox_shader: shader/skybox/fragment.spv shader/skybox/vertex.spv

shader/skybox/fragment.spv: shader/skybox/fragment.glsl
	glslc -fshader-stage=frag shader/skybox/fragment.glsl -o shader/skybox/fragment.spv

shader/skybox/vertex.spv: shader/skybox/vertex.glsl
	glslc -fshader-stage=vert shader/skybox/vertex.glsl -o shader/skybox/vertex.spv

clean:
	rm shader/example_fragment.spv
	rm shader/example_vertex.spv
	rm shader/skybox/fragment.spv
	rm shader/skybox/vertex.spv
