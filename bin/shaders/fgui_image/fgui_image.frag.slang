// FGUI Image Fragment Shader — Slang version

struct FSInput {
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;
    float4 position : SV_Position;
};

[[vk::binding(1, 0)]] SamplerState         image_sampler;
[[vk::binding(2, 0)]] Texture2D<float4>    image_tex;

[shader("fragment")]
float4 main(FSInput input) : SV_Target {
    return image_tex.Sample(image_sampler, input.uv);
}
