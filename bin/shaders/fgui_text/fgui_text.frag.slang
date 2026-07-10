// FGUI Text SDF Fragment Shader
struct FSInput {
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;  // .x=baseSmoothing, .y=sizeScale, .z=effectType
    float4 position : SV_Position;
};

[[vk::binding(1, 0)]] SamplerState        image_sampler;
[[vk::binding(2, 0)]] Texture2DArray<float> image_tex;

float4 srcOverBlend(float4 src, float4 dst) {
    float3 rgb = src.rgb * src.a + dst.rgb * dst.a * (1.0 - src.a);
    float  a   = src.a + dst.a * (1.0 - src.a);
    return float4(rgb / max(a, 0.001), a);
}

[shader("fragment")]
float4 main(FSInput input) : SV_Target
{
    // 柔边 = baseSmoothing × sizeScale
    // sizeScale = sourceSize / fontSize (CPU 侧传入)
    float smoothR = max(input.props.x * input.props.y, 0.03);
    float sMin = 0.5 - smoothR * 0.5;
    float sMax = 0.5 + smoothR * 2.0;

    float dist = image_tex.Sample(image_sampler, float3(input.uv, 0));
    float contentAlpha = smoothstep(sMin, sMax, dist);
    float4 contentColor = float4(input.color.rgb, input.color.a * contentAlpha);

    uint effectType = uint(input.props.z);

    if (effectType == 0u) {
        return contentColor;
    } else if (effectType == 1u) {
        // props.w = outline width
        float outlineW = max(input.props.w, smoothR);
        float outlineAlpha = smoothstep(sMin - outlineW, sMin, dist);
        outlineAlpha -= contentAlpha;
        float4 outlineColor = float4(input.color.rgb, outlineAlpha);
        return srcOverBlend(outlineColor, contentColor);
    } else if (effectType == 2u) {
        float distOffset = image_tex.Sample(image_sampler, float3(input.uv - 0.001, 0));
        float shadowAlpha = smoothstep(sMin - 0.2, sMin, distOffset);
        shadowAlpha -= contentAlpha;
        float4 shadowColor = float4(input.color.rgb, shadowAlpha);
        return srcOverBlend(shadowColor, contentColor);
    } else if (effectType == 3u) {
        float innerAlpha = smoothstep(sMax + 0.15, sMax, dist);
        innerAlpha = min(innerAlpha, contentAlpha);
        float4 innerColor = float4(input.color.rgb, innerAlpha);
        return srcOverBlend(innerColor, contentColor);
    }

    return contentColor;
}
