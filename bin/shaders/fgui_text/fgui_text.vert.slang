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
    [[vk::location(3)]] uint   propIndex : TEXCOORD3;
    [[vk::location(4)]] uint   packedParamSDF : TEXCOORD4;
    float4 position : SV_Position;
};

struct InstanceData
{
    float4x4 transform;
    uint     colorPacked;
    uint     packedProps; // low 8 bits: gray, next 8 bits: hdr
    uint     outlineColorPacked; // packed RGBA8
    uint     packedParamSDF; // low 8 bits: outlineWidth, next 8 bits: shadowOffsetX, next 8 bits: shadowOffsetY, top 8 bits: effectType
};

[[vk::binding(0, 0)]] cbuffer args {
    InstanceData imageDatas[512];
}

[[vk::binding(0, 1)]] cbuffer global {
    float4x4 vp;
    float4x4 batchWorld;
}

float4 unpackColor(uint c) {
    float r = float((c) & 255u) / 255.0;
    float g = float((c >> 8) & 255u) / 255.0;
    float b = float((c >> 16) & 255u) / 255.0;
    float a = float((c >> 24) & 255u) / 255.0;
    return float4(r, g, b, a);
}

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;

    uint idx = input.propIndex;

    output.position = mul(vp, mul(batchWorld, mul(imageDatas[idx].transform, float4(input.position, 1.0))));

    output.uv = input.uv;

    // pass through propIndex so fragment shader can index imageDatas
    output.propIndex = input.propIndex;

    float r = float( input.packedColor        & 0xffu) / 255.0;
    float g = float((input.packedColor >>  8) & 0xffu) / 255.0;
    float b = float((input.packedColor >> 16) & 0xffu) / 255.0;
    float a = float((input.packedColor >> 24) & 0xffu) / 255.0;
    float4 vertexColor = float4(r, g, b, a);
    output.color = vertexColor * unpackColor(imageDatas[idx].colorPacked);

    output.props = float4(0.0, 0.0, 0.0, 0.0);
    output.packedParamSDF = imageDatas[idx].packedParamSDF;

    return output;
}
