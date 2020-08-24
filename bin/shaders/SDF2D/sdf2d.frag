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
    // uint    exColorMask;
    // float   exExtent;
    // float   extraParam3;
    // float   extraParam4;
};

layout ( location = 0 ) out vec4 outFragColor;

/*
Fa = 1; Fb = 1 – αs
co = αs x Cs + αb x Cb x (1 – αs)
αo = αs + αb x (1 – αs)
9.1.5. Destination Over
*/

vec4 srcOverBlend( vec4 src, vec4 dst) {
    float factorA = 1;
    float factorB = 1 - src.a;
    vec3 rgb = src.a * src.rgb + dst.a * dst.b * (1-src.a);
    float alpha = src.a + dst.a * (1-src.a);
    vec4 rst = vec4( rgb, alpha ); 
    return rst;
}

void main() {
    vec4 sampledColor = texture( sampler2DArray(TexArray, TexArraySampler), vec3(frag_uv, texArrayLayerIndex) );
    //float centralAlpha = (smoothstepMin + smoothstepMax)/2;
    float contentAlpha = smoothstep( smoothstepMin, smoothstepMax, sampledColor.r );
    vec4 contentColor = vec4( 1.0f, 1.0f, 1.0f, contentAlpha);
    vec4 colorMask = uint32ToVec4(colorMask);
    contentColor = contentColor * colorMask;
    // outFragColor = contentColor;
    float featheredAlpha = smoothstep(smoothstepMin-0.15, smoothstepMin,sampledColor.r );
    featheredAlpha = featheredAlpha - contentAlpha; // 把标准字内容从 featherd 像素里剔除
    float outlineAlpha = smoothstep(0.0, 0.1,featheredAlpha );  // feather 改描边( 不改描边就是阴影 )
    vec4 outlineColor = vec4(0.0f, 0.0f, 0.0f, outlineAlpha);
    //
    vec4 sampledOffsetColor = texture( sampler2DArray(TexArray, TexArraySampler), vec3(frag_uv - vec2(0.0012, 0.0012), texArrayLayerIndex) );
    float featheredOffsetAlpha = smoothstep(smoothstepMin-0.2, smoothstepMin,sampledOffsetColor.r );
    featheredOffsetAlpha = featheredOffsetAlpha - contentAlpha; // 把标准字内容从 featherd 像素里剔除
    vec4 shadowColor = vec4(0.0f, 0.0f, 0.0f, featheredOffsetAlpha);
    // 混合，这结果就是描边了
    vec4 contentOutlineColor = srcOverBlend( shadowColor, contentColor );
    // vec4 contentOutlineColor = srcOverBlend( outlineColor, contentColor );
    // 求内描边的alpha值，外部为 1，内部为 0
    float innerAlpha = smoothstep( smoothstepMax+0.1, smoothstepMax, sampledColor.r );
    // 渲染出来的是镂空字模，需要弄成字框
    innerAlpha = min( innerAlpha, contentAlpha);
    vec4 innerColor = vec4(1.0f, 0.0f, 0.0f, innerAlpha);
    // 弄成字框就可以直接blend到字上了
    outFragColor = srcOverBlend( innerColor, contentOutlineColor );
}