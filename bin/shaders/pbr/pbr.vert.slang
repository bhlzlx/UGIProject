// PBR Vertex Shader — 简单版 (一个 Renderable = 一个 sphere)

struct VSInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal   : NORMAL;
    [[vk::location(2)]] float2 uv       : TEXCOORD0;
};

struct VSOutput {
    [[vk::location(0)]] float3 worldPos  : TEXCOORD0;
    [[vk::location(1)]] float3 worldNorm : TEXCOORD1;
    [[vk::location(2)]] float2 uv        : TEXCOORD2;
    [[vk::location(3)]] uint   matIndex  : TEXCOORD3;
    float4 position : SV_Position;
};

// set=0, bind=0: 场景 (VP + 相机)
[[vk::binding(0, 0)]] cbuffer SceneData {
    float4x4 viewProj;
    float3   cameraPos;
}

// set=0, bind=1: 单个球体的 model + 材质索引
[[vk::binding(1, 0)]] cbuffer ModelData {
    float4x4 model;
    uint     materialIndex;
}

[shader("vertex")]
VSOutput main(VSInput input) {
    VSOutput o;
    float4 wp = mul(model, float4(input.position, 1.0));
    o.worldPos  = wp.xyz;
    o.worldNorm = normalize(mul((float3x3)model, input.normal));
    o.uv        = input.uv;
    o.matIndex  = materialIndex;
    o.position  = mul(viewProj, wp);
    return o;
}
