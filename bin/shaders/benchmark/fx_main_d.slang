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
float4 fragmentMainD(FSInput input) : SV_Target
{
    float4 texColor = imageTex.Sample(imageSampler, input.uv);
    float3 color = texColor.rgb;
    // 链 D: 冷暖色混合 + ToneMapping 对比
    float3 warm = fx_colorTemp(color, 0.6);
    float3 cool = fx_colorTemp(color, -0.4);
    color = fx_blendSoftLight(warm, cool);
    color = lerp(color, fx_sepia(color), 0.15);
    color = fx_lottes(color);
    color = fx_liftGammaGain(color, float3(-0.05,-0.03,-0.02), float3(1.1,1.05,1.0), float3(1.05,1.0,0.95));
    color = fx_crtEffect(color, input.uv, 240.0);
    color = fx_saturation(color, 1.15);
    return float4(fx_sat(color), texColor.a);
}
