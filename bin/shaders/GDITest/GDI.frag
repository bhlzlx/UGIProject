#version 450
#define VULKAN 100

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 frag_color;

// layout ( set = 0, binding = 1 ) uniform sampler triSampler;
// layout ( set = 0, binding = 2 ) uniform texture2D triTexture;

layout ( location = 0 ) out vec4 outFragColor;

void main() {
    outFragColor = frag_color; //texture( sampler2D(triTexture, triSampler), frag_uv);
}