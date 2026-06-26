// Triangle Vertex Shader — Slang version
struct VSInput {
    [[vk::location(0)]] float2 position : POSITION;
    [[vk::location(1)]] float2 uv       : TEXCOORD0;
};

struct VSOutput {
    [[vk::location(0)]] float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};

[[vk::binding(0, 0)]] cbuffer Argument1 {
    float4 col1;
    float4 col2;
}

[shader("vertex")]
VSOutput main(VSInput input) {
    VSOutput o;
    float x = dot(float3(input.position.x, input.position.y, 1.0), col1.xyz);
    float y = dot(float3(input.position.x, input.position.y, 1.0), col2.xyz);
    o.position = float4(x, y, 1.0, 1.0);
    o.uv = input.uv;
    return o;
}
