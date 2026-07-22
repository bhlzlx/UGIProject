// =============================================================================
//  FGUI Image Vertex Shader — Slang 版本
//  编译: slangc fgui_image.vert.slang -target spirv -entry main -o fgui_image.vert.spv
// =============================================================================

// ---- 顶点输入 ----
struct VSInput
{
    [[vk::location(0)]] float3 position   : POSITION;
    [[vk::location(1)]] uint    packedColor : COLOR;
    [[vk::location(2)]] float2 uv         : TEXCOORD0;
    [[vk::location(3)]] uint    propIndex  : TEXCOORD1;
};

// ---- 顶点输出 (→ 片元输入) ----
struct VSOutput
{
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;
    [[vk::location(3)]] uint   propIndex : TEXCOORD3;
    float4 position : SV_Position;       // gl_Position 等价物
};

// ---- 实例数据 ----
struct InstanceData
{
    float4x4 transform;
    uint     colorPacked;       // packed RGBA 8-bit per channel
    uint     packedProps;       // low 8 bits: gray, next 8 bits: hdr
    uint     outlineColorPacked; // packed RGBA8 (unused by image, but present for UBO alignment)
    uint     packedParamSDF;    // unused by image, but present for UBO alignment
};

// ---- Uniform Buffers ----
// set=0, binding=0 — 实例数组 (最多512个)
[[vk::binding(0, 0)]] cbuffer args {
    InstanceData imageDatas[512];
}

// set=1, binding=0 — 全局 VP + Batch World 矩阵
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

// =============================================================================
//  入口点
// =============================================================================

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;

    uint idx = input.propIndex;

    // VP * BatchWorld * LocalTransform * Position
    // imageDatas[idx].transform 现在是相对于所属 batch 节点的局部矩阵
    output.position = mul(vp, mul(batchWorld, mul(imageDatas[idx].transform, float4(input.position, 1.0))));

    output.uv = input.uv;

    // pass through propIndex so fragment shader can index imageDatas if needed
    output.propIndex = input.propIndex;

    // 解包顶点 RGBA 颜色，再乘上 instance 颜色
    float r = float( input.packedColor        & 0xffu) / 255.0;
    float g = float((input.packedColor >>  8) & 0xffu) / 255.0;
    float b = float((input.packedColor >> 16) & 0xffu) / 255.0;
    float a = float((input.packedColor >> 24) & 0xffu) / 255.0;
    float4 vertexColor = float4(r, g, b, a);
    output.color = vertexColor * unpackColor(imageDatas[idx].colorPacked);

    uint packed = imageDatas[idx].packedProps;
    output.props = float4(float(packed & 0xFFu) / 255.0, float((packed >> 8) & 0xFFu) / 255.0, 0.0, 0.0);

    return output;
}
