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
    float4 position : SV_Position;       // gl_Position 等价物
};

// ---- 实例数据 ----
struct InstanceData
{
    float4x4 transform;
    float4   color;
    float4   props;     // .x=gray, .y=strength, .z=mode
};

// ---- Uniform Buffers ----
// set=0, binding=0 — 实例数组 (最多512个)
[[vk::binding(0, 0)]] cbuffer args {
    InstanceData imageDatas[512];
}

// set=1, binding=0 — 全局 VP 矩阵
[[vk::binding(0, 1)]] cbuffer global {
    float4x4 vp;
}

// =============================================================================
//  入口点
// =============================================================================

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;

    uint idx = input.propIndex;

    // VP * Transform * Position
    output.position = mul(vp, mul(imageDatas[idx].transform, float4(input.position, 1.0)));

    output.uv = input.uv;

    // 解包 RGBA 颜色
    float a = float((input.packedColor >> 24) & 0xffu) / 255.0;
    float r = float((input.packedColor >> 16) & 0xffu) / 255.0;
    float g = float((input.packedColor >>  8) & 0xffu) / 255.0;
    float b = float( input.packedColor        & 0xffu) / 255.0;
    output.color = float4(r, g, b, a);

    output.props = imageDatas[idx].props;

    return output;
}
