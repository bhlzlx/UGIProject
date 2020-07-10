#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 coord;

layout (location = 0) out vec2 frag_uv;

out gl_PerVertex  {
    vec4 gl_Position;
};

layout( set = 0, binding = 0 ) uniform Argument {
	mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
	gl_Position = vec4( position, 1.0f ) * model * view * projection;
	frag_uv = coord;
}
