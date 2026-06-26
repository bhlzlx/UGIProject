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
float4 fragmentMainC(FSInput input) : SV_Target
{
    float4 texColor = imageTex.Sample(imageSampler, input.uv);
    float3 color = texColor.rgb;
    // 链 C: SDF 2D 图案 + 后处理
    float2 uv2 = input.uv;
    float star = abs(fx_sdStar5(uv2*2.0-1.0, 0.5, 0.2));
    float heart = abs(fx_sdHeart((uv2-0.5)*3.0));
    float d = min(star, heart);
    float3 overlay = float3(smoothstep(0.05,0,d),smoothstep(0.03,0,d)*0.5,smoothstep(0.02,0,d)*0.2);
    color = fx_blendScreen(color, overlay);
    color = fx_filmic(color);
    color = fx_vibrance(color, 0.35);
    color = lerp(color, fx_grayscale(color), 0.2);
    color += fx_filmGrain(input.uv, 1.5, 0.03);
    return float4(fx_sat(color), texColor.a);
}
