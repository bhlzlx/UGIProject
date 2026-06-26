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
float4 fragmentMainB(FSInput input) : SV_Target
{
    float4 texColor = imageTex.Sample(imageSampler, input.uv);
    float3 color = texColor.rgb;
    // 链 B: 噪声纹理 + 图案覆盖 + 混合
    float n = fx_fbm2D(input.uv * 6.0, 5);
    color = fx_blendOverlay(color, float3(n*0.6, n*0.3, n*0.8));
    color *= lerp(1.0, fx_vignetteSquare(input.uv, 0.6, 0.25), 0.5);
    color = fx_hable(color);
    color = fx_colorTemp(color, 0.3);
    color = fx_contrast(color, 0.2);
    return float4(fx_sat(color), texColor.a);
}
