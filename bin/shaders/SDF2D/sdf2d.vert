#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 Uv;

layout (location = 0) out vec2 outputUV;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

layout( set = 0, binding = 0 ) uniform Transform {
    vec4 col1;
    vec4 col2;
};

void main() {
    float x = dot( vec3( Position.x, Position.y, 1.0f ), col1.xyz);
    float y = dot( vec3( Position.x, Position.y, 1.0f ), col2.xyz);
	gl_Position = vec4( x, y, 1.0f, 1.0f);
	outputUV = Uv;
}
