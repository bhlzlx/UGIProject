#version 450
#define VULKAN 100

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4  fragColor;
layout (location = 1) in float fragGray;
layout (location = 2) in vec4  fragScreenScissor;

layout ( location = 0 ) out vec4 outFragColor;

float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

void main() {
    float sdfValue = sdBox( fragScreenScissor.xy, fragScreenScissor.zw ) / 8.f;
    float sdfAlpha = (1.0f - sdfValue);
    outFragColor = fragColor;
    outFragColor.a = outFragColor.a * clamp(0.0, 1.0,sdfAlpha);
}