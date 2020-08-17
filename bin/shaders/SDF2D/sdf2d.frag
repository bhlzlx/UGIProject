#version 450
#define VULKAN 100

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

vec4 uint32ToVec4( uint val ) {
    return vec4( (val>>24)/255.0f, (val>>16&0xff)/255.0f, (val>>8&0xff)/255.0f, (val&0xff)/255.0f );
}

layout (location = 0) in vec2 frag_uv;

layout ( set = 0, binding = 1 ) uniform sampler triSampler;
layout ( set = 0, binding = 2 ) uniform texture2DArray  triTexture;

layout ( set = 0, binding = 3 ) uniform SDFArgument {
    float   sdfMin;
    float   sdfMax;
    float   layerIndex;
    uint    colorMask;
};

layout ( location = 0 ) out vec4 outFragColor;

void main() {
    vec4 color = texture( sampler2DArray(triTexture, triSampler), vec3(frag_uv, layerIndex) );
    float value = smoothstep(  sdfMin, sdfMax, color.r );
    color = vec4( value, value, value, value );
    vec4 colorMask = uint32ToVec4( colorMask );
    outFragColor = color * colorMask;
}