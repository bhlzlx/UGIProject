// Triangle Fragment Shader — Slang version
struct FSInput {
    [[vk::location(0)]] float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};

[[vk::binding(1, 0)]] SamplerState         triSampler;
[[vk::binding(2, 0)]] Texture2D<float4>    triTexture;

[shader("fragment")]
float4 main(FSInput input) : SV_Target
{
    return triTexture.Sample(triSampler, input.uv);
}
