#version 450
#define VULKAN 100

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

vec4 uint32ToVec4( uint val ) {
    return vec4( (val>>24)/255.0f, (val>>16&0xff)/255.0f, (val>>8&0xff)/255.0f, (val&0xff)/255.0f );
}

layout (location = 0) in vec3  _uvw;
layout( location = 1) in vec4  _color;
layout( location = 2) in flat uint  _type;
layout( location = 3) in vec4  _effectColor;

layout ( set = 0, binding = 4 ) uniform sampler             TexArraySampler;
layout ( set = 0, binding = 5 ) uniform texture2DArray      TexArray;

const float smoothStepMin = 0.485;
const float smoothStepMax = 0.51;
const float outlineThreshold = 0.12;
const float shadowOffset = 0.0012;
const float shadowThreshold = 0.2;
const float innerGlowThreshold = 0.15;

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
    vec3 rgb = src.rgb * src.a  + dst.rgb * dst.a * (1-src.a);
    float alpha = src.a + dst.a * (1-src.a);
    vec4 rst = vec4( rgb/alpha, alpha ); 
    return rst;
}

void main() {
    vec4 sampledColor = texture( sampler2DArray(TexArray, TexArraySampler), _uvw );

    float contentAlpha = smoothstep( smoothStepMin, smoothStepMax, sampledColor.r );
    vec4 contentColor = vec4( _color.rgb, contentAlpha);

    if( _type == 0 )  { // 没有任何效果
        outFragColor = contentColor;
    } else if( _type == 1 ) { // 描边
        float featheredAlpha = smoothstep( smoothStepMin-outlineThreshold , smoothStepMin,sampledColor.r );
        featheredAlpha = featheredAlpha - contentAlpha; // 把标准字内容从 featherd 像素里剔除
        float outlineAlpha = smoothstep(0.0, 0.1,featheredAlpha );  // feather 改描边( 不改描边就是阴影 ) 这个0.1可以写死，无所谓的
        vec4 outlineColor = vec4(_effectColor.rgb, outlineAlpha);
        // 混合！
        outFragColor = srcOverBlend( outlineColor, contentColor );
    } else if( _type == 2 ) { // 阴影
        vec4 sampledOffsetColor = texture( sampler2DArray(TexArray, TexArraySampler), vec3(_uvw.xy - vec2(shadowOffset, shadowOffset), _uvw.z) );
        float featheredOffsetAlpha = smoothstep(smoothStepMin-shadowThreshold, smoothStepMin,sampledOffsetColor.r );
        featheredOffsetAlpha = featheredOffsetAlpha - contentAlpha; // 把标准字内容从 featherd 像素里剔除
        vec4 shadowColor = vec4(_effectColor.rgb, featheredOffsetAlpha);
        // 混合！
        outFragColor = srcOverBlend( shadowColor, contentColor );
    } else if( _type == 3 ) { // 内发光
        float innerAlpha = smoothstep( smoothStepMax+innerGlowThreshold, smoothStepMax, sampledColor.r );
        // 渲染出来的是镂空字模，需要弄成描字框
        innerAlpha = min( innerAlpha, contentAlpha);
        vec4 innerColor = vec4(_effectColor.rgb, innerAlpha);
        // 弄成字框就可以直接blend到字上了，混合！
        outFragColor = srcOverBlend( innerColor, contentColor );
    }
    // outFragColor = contentColor;
    // float featheredAlpha = smoothstep(smoothStepMin-0.15, smoothStepMin,sampledColor.r );
    // featheredAlpha = featheredAlpha - contentAlpha; // 把标准字内容从 featherd 像素里剔除
    // float outlineAlpha = smoothstep(0.0, 0.1,featheredAlpha );  // feather 改描边( 不改描边就是阴影 )
    // vec4 outlineColor = vec4(0.0f, 0.0f, 0.0f, outlineAlpha);
    // //
    // vec4 sampledOffsetColor = texture( sampler2DArray(TexArray, TexArraySampler), vec3(_uv - vec2(0.0012, 0.0012), texArrayLayerIndex) );
    // float featheredOffsetAlpha = smoothstep(smoothStepMin-0.2, smoothStepMin,sampledOffsetColor.r );
    // featheredOffsetAlpha = featheredOffsetAlpha - contentAlpha; // 把标准字内容从 featherd 像素里剔除
    // vec4 shadowColor = vec4(0.0f, 0.0f, 0.0f, featheredOffsetAlpha);
    // // 混合，这结果就是描边了
    // vec4 contentOutlineColor = srcOverBlend( shadowColor, contentColor );
    // // vec4 contentOutlineColor = srcOverBlend( outlineColor, contentColor );
    // // 求内描边的alpha值，外部为 1，内部为 0
    // float innerAlpha = smoothstep( smoothStepMax+0.1, smoothStepMax, sampledColor.r );
    // // 渲染出来的是镂空字模，需要弄成字框
    // innerAlpha = min( innerAlpha, contentAlpha);
    // vec4 innerColor = vec4(1.0f, 0.0f, 0.0f, innerAlpha);
    // // 弄成字框就可以直接blend到字上了 
    // outFragColor = srcOverBlend( innerColor, contentOutlineColor );
}