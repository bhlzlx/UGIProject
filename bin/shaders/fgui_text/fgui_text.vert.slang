// =============================================================================
//  FGUI Text SDF Vertex Shader
//  与 fgui_image.vert 相同 —— instanced 渲染，每个字符一个 instance
// =============================================================================

struct VSInput
{
    [[vk::location(0)]] float3 position   : POSITION;
    [[vk::location(1)]] uint    packedColor : COLOR;
    [[vk::location(2)]] float2 uv         : TEXCOORD0;
    [[vk::location(3)]] uint    propIndex  : TEXCOORD1;
};

struct VSOutput
{
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;
    float4 position : SV_Position;
};

struct InstanceData
{
    float4x4 transform;
    float4   color;
    float4   props;     // .x=sdfSmooth, .y=outline, .z=effect
};

[[vk::binding(0, 0)]] cbuffer args {
    InstanceData imageDatas[512];
}

[[vk::binding(0, 1)]] cbuffer global {
    float4x4 vp;
    float4x4 batchWorld;
}

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;

    uint idx = input.propIndex;

    output.position = mul(vp, mul(batchWorld, mul(imageDatas[idx].transform, float4(input.position, 1.0))));

    output.uv = input.uv;

    float r = float( input.packedColor        & 0xffu) / 255.0;
    float g = float((input.packedColor >>  8) & 0xffu) / 255.0;
    float b = float((input.packedColor >> 16) & 0xffu) / 255.0;
    float a = float((input.packedColor >> 24) & 0xffu) / 255.0;
    output.color = float4(r, g, b, a);
    output.color = output.color * imageDatas[idx].color;

    output.props = imageDatas[idx].props;

    return output;
}
