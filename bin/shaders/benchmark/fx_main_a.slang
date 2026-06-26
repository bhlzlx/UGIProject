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
float4 fragmentMainA(FSInput input) : SV_Target
{
    float4 texColor = imageTex.Sample(imageSampler, input.uv);
    float3 color = texColor.rgb;
    // 链 A: SDF 3D 叠加 + 色彩
    float3 p = float3(input.uv*2.0-1.0, 0.5);
    float d = fx_sdSphere(p, 0.4);
    d = fx_opSmoothUnion(d, fx_sdBox3D(p, float3(0.3,0.2,0.2)), 0.15);
    d = fx_opSmoothUnion(d, fx_sdTorus(p-float3(0.2,-0.1,0), float2(0.15,0.05)), 0.1);
    float3 sdfColor = float3(0.2,0.4,1.0);
    color = lerp(color, sdfColor, smoothstep(0.02,0.0,abs(d)));
    color = fx_aces(color);
    color = fx_vibrance(color, 0.4);
    color = fx_saturation(color, 1.2);
    color *= fx_vignette(input.uv, 0.82, 0.3);
    return float4(fx_sat(color), texColor.a);
}
