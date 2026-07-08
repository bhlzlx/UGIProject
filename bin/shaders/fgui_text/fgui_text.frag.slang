// FGUI Text SDF Fragment Shader — 参考 SDF2D
struct FSInput {
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;  // .x=sdfSmooth, .y=outlineWidth, .z=effectType
    float4 position : SV_Position;
};

[[vk::binding(1, 0)]] SamplerState        image_sampler;
[[vk::binding(2, 0)]] Texture2DArray<float> image_tex;  // SDF 图集，单通道 R8

static const float kSmoothMin  = 0.485;
static const float kSmoothMax  = 0.51;

float4 srcOverBlend(float4 src, float4 dst) {
    float3 rgb = src.rgb * src.a + dst.rgb * dst.a * (1.0 - src.a);
    float  a   = src.a + dst.a * (1.0 - src.a);
    return float4(rgb / max(a, 0.001), a);
}

[shader("fragment")]
float4 main(FSInput input) : SV_Target
{
    float dist = image_tex.Sample(image_sampler, float3(input.uv, 0));
    float contentAlpha = smoothstep(kSmoothMin, kSmoothMax, dist);
    float4 contentColor = float4(input.color.rgb, input.color.a * contentAlpha);

    uint effectType = uint(input.props.z);

    if (effectType == 0u) {
        // 基础文字
        return contentColor;
    } else if (effectType == 1u) {
        // 外描边
        float outlineW = max(input.props.y, 0.01);
        float outlineAlpha = smoothstep(kSmoothMin - outlineW, kSmoothMin, dist);
        outlineAlpha -= contentAlpha;
        outlineAlpha = smoothstep(0.0, 0.1, outlineAlpha);
        float4 outlineColor = float4(input.color.rgb, outlineAlpha);
        return srcOverBlend(outlineColor, contentColor);
    } else if (effectType == 2u) {
        // 阴影 (偏移半像素)
        float distOffset = image_tex.Sample(image_sampler, float3(input.uv - 0.001, 0));
        float shadowAlpha = smoothstep(kSmoothMin - 0.2, kSmoothMin, distOffset);
        shadowAlpha -= contentAlpha;
        float4 shadowColor = float4(input.color.rgb, shadowAlpha);
        return srcOverBlend(shadowColor, contentColor);
    } else if (effectType == 3u) {
        // 内发光
        float innerAlpha = smoothstep(kSmoothMax + 0.15, kSmoothMax, dist);
        innerAlpha = min(innerAlpha, contentAlpha);
        float4 innerColor = float4(input.color.rgb, innerAlpha);
        return srcOverBlend(innerColor, contentColor);
    }

    return contentColor;
}
