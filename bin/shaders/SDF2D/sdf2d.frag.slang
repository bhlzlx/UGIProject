// SDF2D Fragment Shader — Slang version
struct FSInput {
    [[vk::location(0)]] float3 uvw         : TEXCOORD0;
    [[vk::location(1)]] float4 color       : COLOR0;
    [[vk::location(2)]] nointerpolation uint type : TEXCOORD2;
    [[vk::location(3)]] float4 effectColor : COLOR3;
    float4 position : SV_Position;
};

[[vk::binding(4, 0)]] SamplerState          TexArraySampler;
[[vk::binding(5, 0)]] Texture2DArray<float4> TexArray;

static const float smoothStepMin   = 0.485;
static const float smoothStepMax   = 0.51;
static const float outlineThreshold = 0.12;
static const float shadowOffset    = 0.0012;
static const float shadowThreshold = 0.2;
static const float innerGlowThreshold = 0.15;

float4 srcOverBlend(float4 src, float4 dst) {
    float3 rgb   = src.rgb * src.a + dst.rgb * dst.a * (1.0 - src.a);
    float  alpha = src.a + dst.a * (1.0 - src.a);
    return float4(rgb / max(alpha, 0.001), alpha);
}

[shader("fragment")]
float4 main(FSInput input) : SV_Target {
    float4 sampled = TexArray.Sample(TexArraySampler, input.uvw);
    float contentAlpha = smoothstep(smoothStepMin, smoothStepMax, sampled.r);
    float4 contentColor = float4(input.color.rgb, contentAlpha);

    if (input.type == 0u) {
        return contentColor;
    } else if (input.type == 1u) {
        float featheredAlpha = smoothstep(smoothStepMin - outlineThreshold, smoothStepMin, sampled.r);
        featheredAlpha -= contentAlpha;
        float outlineAlpha = smoothstep(0.0, 0.1, featheredAlpha);
        float4 outlineColor = float4(input.effectColor.rgb, outlineAlpha);
        return srcOverBlend(outlineColor, contentColor);
    } else if (input.type == 2u) {
        float3 offsetUV = float3(input.uvw.xy - float2(shadowOffset, shadowOffset), input.uvw.z);
        float4 sampledOffset = TexArray.Sample(TexArraySampler, offsetUV);
        float featheredOffsetAlpha = smoothstep(smoothStepMin - shadowThreshold, smoothStepMin, sampledOffset.r);
        featheredOffsetAlpha -= contentAlpha;
        float4 shadowColor = float4(input.effectColor.rgb, featheredOffsetAlpha);
        return srcOverBlend(shadowColor, contentColor);
    } else if (input.type == 3u) {
        float innerAlpha = smoothstep(smoothStepMax + innerGlowThreshold, smoothStepMax, sampled.r);
        innerAlpha = min(innerAlpha, contentAlpha);
        float4 innerColor = float4(input.effectColor.rgb, innerAlpha);
        return srcOverBlend(innerColor, contentColor);
    }
    return contentColor;
}
