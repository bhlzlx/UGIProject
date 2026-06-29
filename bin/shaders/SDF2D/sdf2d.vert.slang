// SDF2D Vertex Shader — Slang version
struct VSInput {
    [[vk::location(0)]] float2 position : POSITION;
    [[vk::location(1)]] float2 uv       : TEXCOORD0;
};

struct VSOutput {
    [[vk::location(0)]] float3 uvw        : TEXCOORD0;
    [[vk::location(1)]] float4 color      : COLOR0;
    [[vk::location(2)]] uint   type       : TEXCOORD2;
    [[vk::location(3)]] float4 effectColor: COLOR3;
    float4 position : SV_Position;
};

struct EffectImp  { uint colorMask; uint effectColor; uint type; uint padding; };
struct TransformImp { float col11, col12, col13; float col21, col22, col23; };

[[vk::binding(0, 0)]] cbuffer Indices   { int4   indices[512]; };
[[vk::binding(1, 0)]] cbuffer Effects   { EffectImp effects[256]; };
[[vk::binding(2, 0)]] cbuffer Transforms { TransformImp transforms[256]; };
[[vk::binding(3, 0)]] cbuffer Context   { float contextWidth; float contextHeight; };

float4 unpackColor(uint val) {
    return float4(float((val>>24)&0xff)/255.0, float((val>>16)&0xff)/255.0, float((val>>8)&0xff)/255.0, float(val&0xff)/255.0);
}

[shader("vertex")]
VSOutput main(VSInput input, uint vid : SV_VertexID) {
    uint tableIndex    = vid / 4u;
    uint indexerIndex  = tableIndex / 2u;
    uint indexerInner  = tableIndex % 2u;

    int4 indexData     = indices[indexerIndex];
    uint styleTrans    = indexData[indexerInner * 2];
    uint layerReserved = indexData[indexerInner * 2 + 1];

    uint styleIndex     = styleTrans & 0xffffu;
    uint transformIndex = styleTrans >> 16u;
    uint layerIndex     = layerReserved & 0xffu;

    EffectImp effect = effects[styleIndex];
    TransformImp t   = transforms[transformIndex];

    float3 col1 = float3(t.col11, t.col12, t.col13);
    float3 col2 = float3(t.col21, t.col22, t.col23);
    float x = dot(float3(input.position.x, input.position.y, 1.0), col1);
    float y = dot(float3(input.position.x, input.position.y, 1.0), col2);

    VSOutput o;
    o.position     = float4(x / contextWidth * 2.0 - 1.0, y / contextHeight * 2.0 - 1.0, 1.0, 1.0);
    o.color        = unpackColor(effect.colorMask);
    o.effectColor  = unpackColor(effect.effectColor);
    o.type         = effect.type & 0xffu;
    o.uvw           = float3(input.uv, float(layerIndex));
    return o;
}
