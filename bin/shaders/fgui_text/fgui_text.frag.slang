// FGUI Text SDF Fragment Shader
struct FSInput {
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;  // .x=baseSmoothing, .y=sizeScale
    [[vk::location(3)]] uint   propIndex : TEXCOORD3;
    [[vk::location(4)]] uint   packedParamSDF : TEXCOORD4;
    float4 position : SV_Position;
};

// InstanceData must match the CPU-side item_args_t layout for args UBO
struct InstanceData
{
    float4x4 transform;
    uint     colorPacked;
    uint     packedProps;
    uint     outlineColorPacked;
    uint     packedParamSDF;
};

[[vk::binding(0, 0)]] cbuffer args {
    InstanceData imageDatas[512];
}

[[vk::binding(1, 0)]] SamplerState        image_sampler;
[[vk::binding(2, 0)]] Texture2DArray<float> image_tex;

float4 srcOverBlend(float4 src, float4 dst) {
    float3 rgb = src.rgb * src.a + dst.rgb * dst.a * (1.0 - src.a);
    float  a   = src.a + dst.a * (1.0 - src.a);
    return float4(rgb / max(a, 0.001), a);
}

static float4 unpackColor(uint c) {
    float r = float((c) & 255u) / 255.0;
    float g = float((c >> 8) & 255u) / 255.0;
    float b = float((c >> 16) & 255u) / 255.0;
    float a = float((c >> 24) & 255u) / 255.0;
    return float4(r, g, b, a);
}

static float unpackGray(uint packed) {
    return float(packed & 0xFFu) / 255.0;
}

static void unpackOutlineParams(uint packed, out uint outlineWidth, out int shadowOffsetX, out int shadowOffsetY, out uint effectType) {
    outlineWidth = packed & 0xFFu;
    shadowOffsetX = int(int8_t((packed >> 8) & 0xFFu));
    shadowOffsetY = int(int8_t((packed >> 16) & 0xFFu));
    effectType = (packed >> 24) & 0xFFu;
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

    uint outlineWidth;
    int shadowOffsetX;
    int shadowOffsetY;
    uint effectType;
    unpackOutlineParams(input.packedParamSDF, outlineWidth, shadowOffsetX, shadowOffsetY, effectType);

    if (effectType == 0u) {
        return contentColor;
    } else if (effectType == 1u) {
        float outlineW = max(float(outlineWidth), smoothR);
        float outlineAlpha = smoothstep(sMin - outlineW, sMin, dist);
        outlineAlpha -= contentAlpha;
        float4 outlineColor = unpackColor(imageDatas[input.propIndex].outlineColorPacked);
        outlineColor.a = outlineAlpha * outlineColor.a;
        return srcOverBlend(outlineColor, contentColor);
    } else if (effectType == 2u) {
        float2 shadowUV = input.uv + float2(float(shadowOffsetX), float(shadowOffsetY)) * 0.001;
        float distOffset = image_tex.Sample(image_sampler, float3(shadowUV, 0));
        float shadowAlpha = smoothstep(sMin - 0.2, sMin, distOffset);
        shadowAlpha -= contentAlpha;
        float4 shadowColor = unpackColor(imageDatas[input.propIndex].outlineColorPacked);
        shadowColor.a = shadowAlpha * shadowColor.a;
        return srcOverBlend(shadowColor, contentColor);
    } else if (effectType == 3u) {
        float innerAlpha = smoothstep(sMax + 0.15, sMax, dist);
        innerAlpha = min(innerAlpha, contentAlpha);
        float4 innerColor = float4(input.color.rgb, innerAlpha);
        return srcOverBlend(innerColor, contentColor);
    }

    return contentColor;
}
