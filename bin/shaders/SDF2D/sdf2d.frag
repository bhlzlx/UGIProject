#version 450
#define VULKAN 100

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

vec4 uint32ToVec4( uint val ) {
    return vec4( (val>>24)/255.0f, (val>>16&0xff)/255.0f, (val>>8&0xff)/255.0f, (val&0xff)/255.0f );
}

layout (location = 0) in vec2 frag_uv;

layout ( set = 0, binding = 1 ) uniform sampler             TexArraySampler;
layout ( set = 0, binding = 2 ) uniform texture2DArray      TexArray;

layout ( set = 0, binding = 3 ) uniform SDFArgument {
    float   smoothstepMin;
    float   smoothstepMax;
    float   texArrayLayerIndex;
    uint    colorMask;
};

layout ( location = 0 ) out vec4 outFragColor;

void main() {
    vec4 sampledColor = texture( sampler2DArray(TexArray, TexArraySampler), vec3(frag_uv, texArrayLayerIndex) );
    float contentAlpha = smoothstep( smoothstepMin, smoothstepMax, sampledColor.r );
    vec4 contentColor = vec4( 1.0f, 1.0f, 1.0f, contentAlpha);
    vec4 colorMask = uint32ToVec4(colorMask);
    contentColor = contentColor * colorMask;
    // outFragColor = contentColor;
    float outlineAlpha = smoothstep(smoothstepMax - 0.12, smoothstepMax,sampledColor.r );
    vec4 outlineColor = vec4(0.0f, 0.0f, 0.0f, outlineAlpha);
    outFragColor = mix( outlineColor, contentColor, contentAlpha);
}