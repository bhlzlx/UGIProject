// =============================================================================
//  FGUI Image Fragment Shader — Slang 版本
//  import fgui_lib 模块，调用库中的后处理函数。
//
//  编译:
//    slangc fgui_image.frag.slang -I . -target spirv -entry main -o fgui_image.frag.spv
//
//  预编译库模块 (可选):
//    slangc fgui_lib.slang -target spirv -o fgui_lib.slang-module
// =============================================================================

import fgui_lib;

// ---- 片元输入 (来自顶点着色器) ----
struct FSInput
{
    [[vk::location(0)]] float2 uv    : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 props : TEXCOORD2;
    float4 position : SV_Position;        // 等价于 gl_FragCoord
};

// ---- 资源绑定 (与原始 GLSL 完全一致) ----
// set=0, binding=1 — 采样器
[[vk::binding(1, 0)]] SamplerState image_sampler;

// set=0, binding=2 — 纹理
[[vk::binding(2, 0)]] Texture2D<float4> image_tex;

// =============================================================================
//  片元入口点  —  硬编码效果链，验证 Slang→SPIR-V 管线可用
//  (原始 GLSL 版: 直接输出纹理，无任何后处理)
// =============================================================================

[shader("fragment")]
float4 main(FSInput input) : SV_Target
{
    float4 texColor = image_tex.Sample(image_sampler, input.uv);
    float3 color    = texColor.rgb;

    // 1. sRGB → Linear
    color = fl_srgbToLinear(color);

    // 2. ACES 色调映射 (明显改变亮度/对比度)
    color = fl_acesNarkowicz(color);

    // 3. 色彩分级: 加饱和度 + 暖色温 + 高对比
    color = fl_vibrance(color,  0.5);
    color = fl_saturation(color, 1.3);
    color = fl_colorTemperature(color, 0.4);
    color = fl_contrast(color, 0.25);

    // 4. 暗角
    color *= fl_vignette(input.uv, 0.8, 0.35);

    // 5. 抖动
    color += fl_bayer8x8(input.position.xy) / 255.0;

    // 6. Linear → sRGB
    color = fl_linearToSRGB(color);
    color = fl_saturate(color);

    return float4(color, texColor.a);
}
