import fx_lib;

struct FSInput {
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;
    float4 position : SV_Position;
};
[[vk::binding(1, 0)]] SamplerState imageSampler;
[[vk::binding(2, 0)]] Texture2D<float4> imageTex;

[shader("fragment")]
float4 fragmentMainE(FSInput input) : SV_Target
{
    float4 texColor = imageTex.Sample(imageSampler, input.uv);
    float3 color = texColor.rgb;
    // 链 E: 噪声扰动 + 色彩偏移
    float warp = fx_domainWarp2D(input.uv * 5.0);
    float2 uvWarped = input.uv + float2(warp, warp*0.7) * 0.02;
    float4 warpedColor = imageTex.Sample(imageSampler, uvWarped);
    color = lerp(color, warpedColor.rgb, 0.3);
    color = fx_reinhard(color);
    color = lerp(color, fx_hueShift(color, 0.05), 0.4);
    color = fx_shadowsMidtonesHighlights(color, float3(0.05,0.02,0.01), float3(0,0,0), float3(-0.03,-0.02,-0.04));
    color *= lerp(1.0, fx_vignette(input.uv, 0.78, 0.4), 0.55);
    color = fx_posterize(color, 32.0);
    color += fx_filmGrain(input.uv, 2.0, 0.02);
    return float4(fx_sat(color), texColor.a);
}
