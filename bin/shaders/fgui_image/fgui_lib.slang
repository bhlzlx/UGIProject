// =============================================================================
//  FGUI Effects Library  —  Slang 版本
// =============================================================================
//  本模块不含入口点，专为 separate compilation / import 设计。
//  调用方: import fgui_lib;
//  编译:  slangc fgui_lib.slang -target none -o fgui_lib.slang-module
//  注意:  不加 module 声明，所有函数默认全局可见，import 即可用
// =============================================================================

// ---- 常量 ----
static const float FL_PI      = 3.14159265358979323846;
static const float FL_INV_PI  = 0.31830988618379067154;
static const float FL_EPSILON = 1e-6;
static const float FL_GOLDEN  = 1.61803398874989484820;

// =============================================================================
//  1. 数学工具
// =============================================================================

float fl_saturate(float x)                     { return clamp(x, 0.0, 1.0); }
float2 fl_saturate(float2 x)                   { return clamp(x, 0.0, 1.0); }
float3 fl_saturate(float3 x)                   { return clamp(x, 0.0, 1.0); }
float4 fl_saturate(float4 x)                   { return clamp(x, 0.0, 1.0); }

float fl_pow2(float x)                         { return x * x; }
float fl_pow3(float x)                         { return x * x * x; }
float fl_pow5(float x)                         { float x2 = x * x; return x2 * x2 * x; }

float fl_rcp(float x)                          { return 1.0 / x; }
float fl_rcpSafe(float x)                      { return 1.0 / max(x, FL_EPSILON); }

float fl_almostIdentity(float x, float m, float n)
{
    if (x > m) return x;
    float a = 2.0 * n - m;
    float b = 2.0 * m - 3.0 * n;
    float t = x / m;
    return (a * x + b) * t * t + n;
}

// =============================================================================
//  2. 亮度计算
// =============================================================================

float fl_luminance(float3 c) {
    return dot(c, float3(0.2126, 0.7152, 0.0722));     // ITU-R BT.709
}

float fl_luminanceBT601(float3 c) {
    return dot(c, float3(0.299, 0.587, 0.114));         // ITU-R BT.601
}

float fl_luminanceBT2020(float3 c) {
    return dot(c, float3(0.2627, 0.6780, 0.0593));      // ITU-R BT.2020
}

float fl_perceptualLuminance(float3 c) {
    float3 w = float3(0.2126, 0.7152, 0.0722);
    return sqrt(dot(c * c, w));
}

// =============================================================================
//  3. Gamma / 色彩空间
// =============================================================================

float fl_srgbToLinear(float c) {
    if (c <= 0.04045) return c / 12.92;
    else              return pow((c + 0.055) / 1.055, 2.4);
}
float3 fl_srgbToLinear(float3 c) { return float3(fl_srgbToLinear(c.x), fl_srgbToLinear(c.y), fl_srgbToLinear(c.z)); }
float4 fl_srgbToLinear(float4 c) { return float4(fl_srgbToLinear(c.xyz), c.w); }

float fl_linearToSRGB(float c) {
    if (c <= 0.0031308) return 12.92 * c;
    else                return 1.055 * pow(c, 1.0 / 2.4) - 0.055;
}
float3 fl_linearToSRGB(float3 c) {
    return lerp(c * 12.92, 1.055 * pow(c, 1.0 / 2.4) - 0.055, step(0.0031308, c));
}
float4 fl_linearToSRGB(float4 c) { return float4(fl_linearToSRGB(c.xyz), c.w); }

float3 fl_rec709ToLinear(float3 c)  { return pow(fl_saturate(c), 2.2); }

float3 fl_gammaEncode(float3 c, float gamma) { return pow(fl_saturate(c), 1.0 / gamma); }
float3 fl_gammaDecode(float3 c, float gamma) { return pow(fl_saturate(c), gamma); }

// =============================================================================
//  4. 色调映射 (13种)
// =============================================================================

float3 fl_reinhard(float3 c)                      { return c / (c + 1.0); }

float3 fl_reinhardExtended(float3 c, float white) {
    float3 num = c * (1.0 + c / (white * white));
    return num / (c + 1.0);
}

float3 fl_reinhardLuma(float3 c) {
    float l  = fl_luminance(c);
    float tm = l / (1.0 + l);
    return c * fl_rcpSafe(l) * tm;
}

float3 fl_acesNarkowicz(float3 c) {
    const float a  = 2.51;
    const float b  = 0.03;
    const float cc = 2.43;
    const float d  = 0.59;
    const float e  = 0.14;
    return fl_saturate((c * (a * c + b)) / (c * (cc * c + d) + e));
}

float3 fl_acesHill(float3 c) {
    float3x3 acesIn = float3x3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    );
    float3x3 acesOut = float3x3(
         1.60475, -0.53108, -0.07367,
        -0.10208,  1.10813, -0.00605,
        -0.00327, -0.07276,  1.07602
    );
    c = mul(acesIn, c);
    c = (c * (c + 0.0245786) - 0.000090537) / (c * (0.983729 * c + 0.4329510) + 0.238081);
    c = mul(acesOut, c);
    return fl_saturate(c);
}

float3 fl_hable(float3 c) {
    const float A = 0.22, B = 0.30, C = 0.10, D = 0.20, E = 0.01, F = 0.30;
    return fl_saturate(((c*(A*c + C*B) + D*E) / (c*(A*c + B) + D*F)) - E/F);
}

float3 fl_lottes(float3 c) {
    float a = 1.6, d = 0.977, hdrMax = 8.0, midIn = 0.18, midOut = 0.267;
    float b  = (-pow(midIn,a) + pow(hdrMax,a)*midOut) / ((pow(hdrMax,a*d) - pow(midIn,a*d))*midOut);
    float c0 = (pow(hdrMax,a*d)*pow(midIn,a) - pow(hdrMax,a)*pow(midIn,a*d)*midOut) / ((pow(hdrMax,a*d) - pow(midIn,a*d))*midOut);
    return fl_saturate(pow(c,a) / (pow(c, a*d) * b + c0));
}

float3 fl_uchimura(float3 x) {
    float P  = 1.0;
    float a  = 1.0;
    float m  = 0.22;
    float l  = 0.4;
    float ct = 1.33;
    float b  = 0.0;
    float l0 = ((P - m) * l) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;
    float3 w0 = 1.0 - smoothstep(0.0, m, x);
    float3 w2 = step(m + l0, x);
    float3 w1 = 1.0 - w0 - w2;
    float3 T  = m * pow(x / m, ct) + b;
    float3 S  = P - (P - S1) * exp(CP * (x - S0));
    float3 L  = m + a * (x - m);
    return T * w0 + L * w1 + S * w2;
}

float3 fl_filmic(float3 c) {
    c = max(0.0, c - 0.004);
    return (c * (6.2 * c + 0.5)) / (c * (6.2 * c + 1.7) + 0.06);
}

float3 fl_amdTonemap(float3 c)       { return c * (c + 2.0) / (c*c + c + 4.0); }

float3 fl_ward(float3 c, float lda)  { return c * pow(lda, 0.5) / (lda + 1.0); }

float3 fl_drago(float3 c, float bias) {
    float l     = max(fl_luminance(c), FL_EPSILON);
    float lMax  = 100.0;
    float logL  = log(l + 1.0);
    float logMax= log(lMax + 1.0);
    float tm    = logL / logMax;
    tm = tm / (log(2.0 + 8.0 * pow(l / lMax, bias)) / log(10.0));
    return c * tm / l;
}

float3 fl_hyperbolic(float3 c)       { return c / (c + 1.0) + 0.05; }

// =============================================================================
//  5. 高斯 & Box 模糊
//  参数: tex(Texture2D), samp(SamplerState), uv, texelSize, weights
// =============================================================================

float fl_gaussianWeight(float x, float sigma) {
    return (1.0 / (sigma * sqrt(2.0 * FL_PI))) * exp(-(x * x) / (2.0 * sigma * sigma));
}

void fl_gaussianWeights5(out float w[5]) {
    w[0] = 0.0545; w[1] = 0.2442; w[2] = 0.4026; w[3] = 0.2442; w[4] = 0.0545;
}
void fl_gaussianWeights9(out float w[9]) {
    w[0]=0.0162; w[1]=0.0540; w[2]=0.1219; w[3]=0.1954; w[4]=0.2250;
    w[5]=0.1954; w[6]=0.1219; w[7]=0.0540; w[8]=0.0162;
}
void fl_gaussianWeights13(out float w[13]) {
    w[0]=0.0062;  w[1]=0.0146;  w[2]=0.0296;  w[3]=0.0542;
    w[4]=0.0886;  w[5]=0.1295;  w[6]=0.1692;  w[7]=0.1295;
    w[8]=0.0886;  w[9]=0.0542;  w[10]=0.0296; w[11]=0.0146; w[12]=0.0062;
}

float3 fl_gaussianBlurH9(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize, float w[9]) {
    float3 r = 0.0;
    r += tex.Sample(samp, uv + float2(-4.0, 0.0) * texelSize).rgb * w[0];
    r += tex.Sample(samp, uv + float2(-3.0, 0.0) * texelSize).rgb * w[1];
    r += tex.Sample(samp, uv + float2(-2.0, 0.0) * texelSize).rgb * w[2];
    r += tex.Sample(samp, uv + float2(-1.0, 0.0) * texelSize).rgb * w[3];
    r += tex.Sample(samp, uv                            ).rgb * w[4];
    r += tex.Sample(samp, uv + float2( 1.0, 0.0) * texelSize).rgb * w[5];
    r += tex.Sample(samp, uv + float2( 2.0, 0.0) * texelSize).rgb * w[6];
    r += tex.Sample(samp, uv + float2( 3.0, 0.0) * texelSize).rgb * w[7];
    r += tex.Sample(samp, uv + float2( 4.0, 0.0) * texelSize).rgb * w[8];
    return r;
}

float3 fl_gaussianBlurV9(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize, float w[9]) {
    float3 r = 0.0;
    r += tex.Sample(samp, uv + float2(0.0, -4.0) * texelSize).rgb * w[0];
    r += tex.Sample(samp, uv + float2(0.0, -3.0) * texelSize).rgb * w[1];
    r += tex.Sample(samp, uv + float2(0.0, -2.0) * texelSize).rgb * w[2];
    r += tex.Sample(samp, uv + float2(0.0, -1.0) * texelSize).rgb * w[3];
    r += tex.Sample(samp, uv                            ).rgb * w[4];
    r += tex.Sample(samp, uv + float2(0.0,  1.0) * texelSize).rgb * w[5];
    r += tex.Sample(samp, uv + float2(0.0,  2.0) * texelSize).rgb * w[6];
    r += tex.Sample(samp, uv + float2(0.0,  3.0) * texelSize).rgb * w[7];
    r += tex.Sample(samp, uv + float2(0.0,  4.0) * texelSize).rgb * w[8];
    return r;
}

float3 fl_gaussianBlurH5(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize, float w[5]) {
    float3 r = 0.0;
    r += tex.Sample(samp, uv + float2(-2.0, 0.0) * texelSize).rgb * w[0];
    r += tex.Sample(samp, uv + float2(-1.0, 0.0) * texelSize).rgb * w[1];
    r += tex.Sample(samp, uv                            ).rgb * w[2];
    r += tex.Sample(samp, uv + float2( 1.0, 0.0) * texelSize).rgb * w[3];
    r += tex.Sample(samp, uv + float2( 2.0, 0.0) * texelSize).rgb * w[4];
    return r;
}

float3 fl_gaussianBlurV5(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize, float w[5]) {
    float3 r = 0.0;
    r += tex.Sample(samp, uv + float2(0.0, -2.0) * texelSize).rgb * w[0];
    r += tex.Sample(samp, uv + float2(0.0, -1.0) * texelSize).rgb * w[1];
    r += tex.Sample(samp, uv                            ).rgb * w[2];
    r += tex.Sample(samp, uv + float2(0.0,  1.0) * texelSize).rgb * w[3];
    r += tex.Sample(samp, uv + float2(0.0,  2.0) * texelSize).rgb * w[4];
    return r;
}

float3 fl_boxBlur3x3(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize) {
    float3 r = 0.0;
    for (int i = -1; i <= 1; i++)
        for (int j = -1; j <= 1; j++)
            r += tex.Sample(samp, uv + float2(i, j) * texelSize).rgb;
    return r * (1.0 / 9.0);
}

float3 fl_boxBlur5x5(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize) {
    float3 r = 0.0;
    for (int i = -2; i <= 2; i++)
        for (int j = -2; j <= 2; j++)
            r += tex.Sample(samp, uv + float2(i, j) * texelSize).rgb;
    return r * (1.0 / 25.0);
}

float3 fl_kawaseBlur(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize, int iter) {
    float2 d = texelSize * float(iter + 1);
    float3 r  = tex.Sample(samp, uv + float2(-0.5, -0.5) * d).rgb;
         r += tex.Sample(samp, uv + float2( 0.5, -0.5) * d).rgb;
         r += tex.Sample(samp, uv + float2(-0.5,  0.5) * d).rgb;
         r += tex.Sample(samp, uv + float2( 0.5,  0.5) * d).rgb;
    return r * 0.25;
}

// =============================================================================
//  6. 锐化 & 边缘检测
// =============================================================================

float3 fl_sharpen3x3(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize, float strength) {
    float3 center = tex.Sample(samp, uv).rgb;
    float3 blur   = fl_boxBlur3x3(tex, samp, uv, texelSize);
    return center + (center - blur) * strength;
}

float3 fl_unsharpMask(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize, float strength, float threshold) {
    float3 color   = tex.Sample(samp, uv).rgb;
    float3 blurred = fl_boxBlur3x3(tex, samp, uv, texelSize);
    float3 mask    = color - blurred;
    if (abs(fl_luminance(mask)) < threshold) mask = 0.0;
    return color + mask * strength;
}

float fl_sobelEdge(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize) {
    float tl = fl_luminance(tex.Sample(samp, uv + float2(-1.0,-1.0) * texelSize).rgb);
    float tc = fl_luminance(tex.Sample(samp, uv + float2( 0.0,-1.0) * texelSize).rgb);
    float tr = fl_luminance(tex.Sample(samp, uv + float2( 1.0,-1.0) * texelSize).rgb);
    float ml = fl_luminance(tex.Sample(samp, uv + float2(-1.0, 0.0) * texelSize).rgb);
    float mr = fl_luminance(tex.Sample(samp, uv + float2( 1.0, 0.0) * texelSize).rgb);
    float bl = fl_luminance(tex.Sample(samp, uv + float2(-1.0, 1.0) * texelSize).rgb);
    float bc = fl_luminance(tex.Sample(samp, uv + float2( 0.0, 1.0) * texelSize).rgb);
    float br = fl_luminance(tex.Sample(samp, uv + float2( 1.0, 1.0) * texelSize).rgb);
    float gx = tl*-1.0 + tr*1.0 + ml*-2.0 + mr*2.0 + bl*-1.0 + br*1.0;
    float gy = tl*-1.0 + tc*-2.0 + tr*-1.0 + bl*1.0 + bc*2.0 + br*1.0;
    return sqrt(gx*gx + gy*gy);
}

float fl_laplacianEdge(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize) {
    float c = fl_luminance(tex.Sample(samp, uv).rgb) * 4.0;
    float n = fl_luminance(tex.Sample(samp, uv + float2(0.0,-1.0) * texelSize).rgb);
    float s = fl_luminance(tex.Sample(samp, uv + float2(0.0, 1.0) * texelSize).rgb);
    float e = fl_luminance(tex.Sample(samp, uv + float2(1.0, 0.0) * texelSize).rgb);
    float w = fl_luminance(tex.Sample(samp, uv + float2(-1.0,0.0) * texelSize).rgb);
    return c - n - s - e - w;
}

// =============================================================================
//  7. 基础调色
// =============================================================================

float3 fl_saturation(float3 c, float s)    { float l = fl_luminance(c); return l + s * (c - l); }
float3 fl_contrast(float3 c, float amount) { return (c - 0.5) * (1.0 + amount) + 0.5; }
float3 fl_brightness(float3 c, float a)    { return c + a; }
float3 fl_exposure(float3 c, float stops)  { return c * exp2(stops); }

float3 fl_vibrance(float3 c, float strength) {
    float l   = fl_luminance(c);
    float mx  = max(max(c.x, c.y), c.z);
    float mn  = min(min(c.x, c.y), c.z);
    float sat = mx - mn;
    return c + strength * (1.0 - sat) * (c - l);
}

// =============================================================================
//  8. 高级调色
// =============================================================================

float3 fl_liftGammaGain(float3 c, float3 lift, float3 gamma, float3 gain) {
    return pow(fl_saturate(gain * (c + lift)), 1.0 / gamma);
}

float3 fl_colorTemperature(float3 c, float shift) {
    float3 warm = float3(1.0, 0.78, 0.50);
    float3 cool = float3(0.50, 0.67, 1.0);
    return c * lerp(cool, warm, shift * 0.5 + 0.5);
}

float3 fl_tint(float3 c, float amount) {
    return c + amount * float3(0.05, -0.05, 0.05);
}

float3 fl_whiteBalance(float3 c, float temp, float tintAmount) {
    return fl_tint(fl_colorTemperature(c, temp), tintAmount);
}

float3 fl_hueShift(float3 c, float angle) {
    float cosA = cos(angle), sinA = sin(angle);
    float k = 1.0 / 3.0;
    float sk = sqrt(k);
    float3x3 rot = float3x3(
        cosA+(1.0-cosA)*k,         (1.0-cosA)*k - sk*sinA,  (1.0-cosA)*k + sk*sinA,
        (1.0-cosA)*k + sk*sinA,    cosA+(1.0-cosA)*k,       (1.0-cosA)*k - sk*sinA,
        (1.0-cosA)*k - sk*sinA,    (1.0-cosA)*k + sk*sinA,  cosA+(1.0-cosA)*k
    );
    return mul(rot, c);
}

float3 fl_channelMixer(float3 c, float3 rCoeff, float3 gCoeff, float3 bCoeff) {
    return float3(dot(c, rCoeff), dot(c, gCoeff), dot(c, bCoeff));
}

float3 fl_offsetPowerSlope(float3 c, float3 offset, float3 power, float3 slope) {
    return pow(fl_saturate(slope * c + offset), power);
}

float3 fl_shadowsMidtonesHighlights(float3 c, float3 shadows, float3 midtones, float3 highlights) {
    float l   = fl_luminance(c);
    float shd = 1.0 - smoothstep(0.0, 0.4, l);
    float hig = smoothstep(0.6, 1.0, l);
    float mid = 1.0 - shd - hig;
    return c + shadows*shd + midtones*mid + highlights*hig;
}

// =============================================================================
//  9. 色彩滤镜
// =============================================================================

float3 fl_grayscale(float3 c)               { float l = fl_luminance(c); return float3(l, l, l); }
float3 fl_grayscaleAvg(float3 c)            { float a = (c.x+c.y+c.z)/3.0; return float3(a,a,a); }
float3 fl_grayscaleMax(float3 c)            { float m = max(max(c.x,c.y),c.z); return float3(m,m,m); }
float3 fl_invert(float3 c)                  { return 1.0 - c; }

float3 fl_sepiaTone(float3 c) {
    float l = fl_luminance(c);
    return float3(l * 1.351, l * 1.203, l * 0.937);
}

float3 fl_solarize(float3 c, float threshold) {
    return lerp(c, fl_invert(c), step(threshold, fl_luminance(c)));
}

float3 fl_posterize(float3 c, float levels) { return floor(c * levels) / (levels - 1.0); }

float3 fl_thresholdBW(float3 c, float cut)  { float l = fl_luminance(c); return step(cut, l); }

float3 fl_lofi(float3 c, float ditherAmount) {
    float l = fl_luminance(c);
    return float3(floor(l*4.0)/3.0, floor(l*4.0)/3.0, floor(l*4.0)/3.0) + ditherAmount * (c - float3(l,l,l));
}

float3 fl_vintageFade(float3 c, float fade) { return lerp(c, float3(0.8,0.75,0.65), fade); }

float3 fl_tealOrange(float3 c, float strength) {
    float l       = fl_luminance(c);
    float3 teal   = float3(0.0, 0.5, 0.5);
    float3 orange = float3(1.0, 0.5, 0.0);
    float3 graded = lerp(teal, orange, l);
    return lerp(c, graded, strength);
}

float3 fl_bleachBypass(float3 c, float opacity) {
    float l       = fl_luminance(c);
    float3 bw     = float3(l,l,l);
    float3 blend   = fl_saturate(2.0 * c * bw);
    return lerp(c, blend, opacity);
}

float3 fl_crossProcess(float3 c) {
    c.x = fl_saturate(c.x * 1.2 - 0.1);
    c.y = fl_saturate(c.y * 0.9 + 0.05);
    c.z = fl_saturate(c.z * 1.1 + 0.05);
    return c;
}

// =============================================================================
// 10. 混合模式
// =============================================================================

float3 fl_blendScreen(float3 base, float3 blend)     { return 1.0 - (1.0-base)*(1.0-blend); }
float3 fl_blendMultiply(float3 base, float3 blend)   { return base * blend; }

float3 fl_blendOverlay(float3 base, float3 blend) {
    bool3 c = base > 0.5;
    return lerp(2.0*base*blend, 1.0-2.0*(1.0-base)*(1.0-blend), c);
}

float3 fl_blendSoftLight(float3 base, float3 blend) {
    float3 lo = 2.0*base*blend + base*base*(1.0-2.0*blend);
    float3 hi = 2.0*base*(1.0-blend) + sqrt(base)*(2.0*blend-1.0);
    return lerp(hi, lo, blend < 0.5);
}

float3 fl_blendHardLight(float3 base, float3 blend)   { return fl_blendOverlay(blend, base); }
float3 fl_blendLinearLight(float3 base, float3 blend) { return base + 2.0*blend - 1.0; }

float3 fl_blendVividLight(float3 base, float3 blend) {
    bool3 c = blend < 0.5;
    float3 lo = 1.0 - (1.0-base) / (2.0*blend + FL_EPSILON);
    float3 hi = base / (2.0*(1.0-blend) + FL_EPSILON);
    return fl_saturate(lerp(hi, lo, c));
}

float3 fl_blendAdd(float3 base, float3 blend)        { return base + blend; }
float3 fl_blendSubtract(float3 base, float3 blend)   { return base - blend; }
float3 fl_blendDifference(float3 base, float3 blend) { return abs(base - blend); }
float3 fl_blendExclusion(float3 base, float3 blend)  { return base + blend - 2.0*base*blend; }
float3 fl_blendColorDodge(float3 base, float3 blend) { return base / (1.0 - blend + FL_EPSILON); }
float3 fl_blendColorBurn(float3 base, float3 blend)  { return 1.0 - (1.0-base) / (blend + FL_EPSILON); }

// =============================================================================
// 11. 暗角
// =============================================================================

float fl_vignette(float2 uv, float intensity, float smoothness) {
    uv = uv * 2.0 - 1.0;
    float dist = length(uv);
    return 1.0 - smoothstep(intensity - smoothness, intensity, dist);
}

float fl_vignetteSquared(float2 uv, float radius, float feather) {
    uv = uv * 2.0 - 1.0;
    float d = dot(uv, uv);
    return 1.0 - smoothstep(radius, radius - feather, d);
}

float fl_vignetteCorrected(float2 uv, float aspect, float intensity, float smoothness) {
    uv = (uv * 2.0 - 1.0) * float2(aspect, 1.0);
    float dist = length(uv);
    return 1.0 - smoothstep(intensity - smoothness, intensity, dist);
}

// =============================================================================
// 12. 抖动
// =============================================================================

float fl_bayer2x2(float2 coord) {
    const float d[4] = { 0.0/4.0, 2.0/4.0, 3.0/4.0, 1.0/4.0 };
    int x = int(fmod(coord.x, 2.0));
    int y = int(fmod(coord.y, 2.0));
    return d[y*2 + x] - 0.5;
}

float fl_bayer4x4(float2 coord) {
    const float d[16] = {
         0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
        12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
         3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
        15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
    };
    int x = int(fmod(coord.x, 4.0));
    int y = int(fmod(coord.y, 4.0));
    return d[y*4 + x] - 0.5;
}

float fl_bayer8x8(float2 coord) {
    const float d[64] = {
         0.0/64.0, 32.0/64.0,  8.0/64.0, 40.0/64.0,  2.0/64.0, 34.0/64.0, 10.0/64.0, 42.0/64.0,
        48.0/64.0, 16.0/64.0, 56.0/64.0, 24.0/64.0, 50.0/64.0, 18.0/64.0, 58.0/64.0, 26.0/64.0,
        12.0/64.0, 44.0/64.0,  4.0/64.0, 36.0/64.0, 14.0/64.0, 46.0/64.0,  6.0/64.0, 38.0/64.0,
        60.0/64.0, 28.0/64.0, 52.0/64.0, 20.0/64.0, 62.0/64.0, 30.0/64.0, 54.0/64.0, 22.0/64.0,
         3.0/64.0, 35.0/64.0, 11.0/64.0, 43.0/64.0,  1.0/64.0, 33.0/64.0,  9.0/64.0, 41.0/64.0,
        51.0/64.0, 19.0/64.0, 59.0/64.0, 27.0/64.0, 49.0/64.0, 17.0/64.0, 57.0/64.0, 25.0/64.0,
        15.0/64.0, 47.0/64.0,  7.0/64.0, 39.0/64.0, 13.0/64.0, 45.0/64.0,  5.0/64.0, 37.0/64.0,
        63.0/64.0, 31.0/64.0, 55.0/64.0, 23.0/64.0, 61.0/64.0, 29.0/64.0, 53.0/64.0, 21.0/64.0
    };
    int x = int(fmod(coord.x, 8.0));
    int y = int(fmod(coord.y, 8.0));
    return d[y*8 + x] - 0.5;
}

float fl_triangleNoise(float2 p) {
    p = frac(p * float2(5.3987, 5.4421));
    p += dot(p.yx, p.xy + float2(21.5351, 14.3137));
    float xy = p.x * p.y;
    return frac(xy * 95.4307) + frac(xy * 75.0496) - 1.0;
}

float fl_hash(float2 p)  { return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453123); }
float fl_hash3(float3 p) { return frac(sin(dot(p, float3(12.9898, 78.233, 45.5432))) * 43758.5453123); }

float fl_interleavedGradientNoise(float2 coord) {
    float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(magic.z * frac(dot(coord, magic.xy)));
}

float fl_blueNoise(float2 coord) {
    return frac(sin(dot(floor(coord), float2(127.1, 311.7))) * 43758.5453);
}

// =============================================================================
// 13. HSV / HSL / YCbCr 转换
// =============================================================================

float3 fl_rgb2hsv(float3 c) {
    float4 K = float4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    float4 p = lerp(float4(c.yz, K.wz), float4(c.yz, K.xy), step(c.z, c.y));
    float4 q = lerp(float4(p.xyw, c.x), float4(c.x, p.yzx), step(p.x, c.x));
    float d = q.x - min(q.w, q.y);
    float e = 1e-10;
    return float3(abs(q.z + (q.w - q.y)/(6.0*d + e)), d/(q.x + e), q.x);
}

float3 fl_hsv2rgb(float3 c) {
    float4 K = float4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, fl_saturate(p - K.xxx), c.y);
}

// ---- HSL (hue2rgb 须在调用者之前定义) ----
float fl_hue2rgb(float p, float q, float t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0/6.0) return p + (q-p) * 6.0 * t;
    if (t < 1.0/2.0) return q;
    if (t < 2.0/3.0) return p + (q-p) * (2.0/3.0 - t) * 6.0;
    return p;
}

float3 fl_rgb2hsl(float3 c) {
    float mn = min(min(c.x,c.y),c.z);
    float mx = max(max(c.x,c.y),c.z);
    float l = (mx+mn)*0.5;
    float s = 0.0;
    float h = 0.0;
    if (mn != mx) {
        float d = mx-mn;
        s = l > 0.5 ? d/(2.0-mx-mn) : d/(mx+mn);
        if (c.x == mx)      { h = (c.y-c.z)/d + (c.y<c.z ? 6.0 : 0.0); }
        else if (c.y == mx) { h = (c.z-c.x)/d + 2.0; }
        else                { h = (c.x-c.y)/d + 4.0; }
        h /= 6.0;
    }
    return float3(h, s, l);
}

float3 fl_hsl2rgb(float3 hsl) {
    if (hsl.y == 0.0) return hsl.z;
    float q = hsl.z < 0.5 ? hsl.z*(1.0+hsl.y) : hsl.z+hsl.y-hsl.z*hsl.y;
    float p = 2.0*hsl.z - q;
    return float3(
        fl_hue2rgb(p, q, hsl.x+1.0/3.0),
        fl_hue2rgb(p, q, hsl.x),
        fl_hue2rgb(p, q, hsl.x-1.0/3.0)
    );
}

// ---- YCbCr ----
float3 fl_rgb2ycbcr(float3 c) {
    return float3(
        dot(c, float3( 0.299,  0.587,  0.114)),
        dot(c, float3(-0.1687,-0.3313, 0.5   )) + 0.5,
        dot(c, float3( 0.5,   -0.4187,-0.0813)) + 0.5
    );
}

float3 fl_ycbcr2rgb(float3 yuv) {
    yuv = yuv - float3(0.0, 0.5, 0.5);
    return float3(
        dot(yuv, float3(1.0,  0.0,      1.402)),
        dot(yuv, float3(1.0, -0.344136,-0.714136)),
        dot(yuv, float3(1.0,  1.772,    0.0))
    );
}

// =============================================================================
// 14. PBR 辅助
// =============================================================================

float3 fl_fresnelSchlick(float3 F0, float VoH) {
    return F0 + (1.0 - F0) * fl_pow5(1.0 - VoH);
}
float fl_fresnelSchlickF0(float ior) {
    float f0 = (ior - 1.0) / (ior + 1.0);
    return f0 * f0;
}
float fl_smithG1(float NoV, float roughness) {
    float a = roughness * roughness;
    float k = a * 0.5;
    return NoV / (NoV * (1.0 - k) + k);
}
float fl_smithG(float NoV, float NoL, float roughness) {
    return fl_smithG1(NoV, roughness) * fl_smithG1(NoL, roughness);
}
float fl_ggxDistribution(float NoH, float roughness) {
    float a    = roughness * roughness;
    float a2   = a * a;
    float NoH2 = NoH * NoH;
    float denom = NoH2 * (a2 - 1.0) + 1.0;
    return a2 / (FL_PI * denom * denom);
}
float fl_disneyDiffuse(float NoV, float NoL, float LoH, float roughness) {
    float a = roughness * roughness;
    float FD90 = 0.5 + 2.0 * LoH * LoH * a;
    return (1.0 + (FD90-1.0)*fl_pow5(1.0-NoL)) * (1.0 + (FD90-1.0)*fl_pow5(1.0-NoV)) * FL_INV_PI;
}

// =============================================================================
// 15. 调试可视化
// =============================================================================

float3 fl_falseColor(float t) {
    return fl_saturate(float3(
        smoothstep(0.5, 1.0, t),
        1.0 - abs(t*2.0 - 1.0),
        smoothstep(0.5, 0.0, t)
    ));
}
float3 fl_visualizeNormals(float3 N)    { return N * 0.5 + 0.5; }
float3 fl_visualizeLuminance(float3 c)  { return fl_falseColor(fl_luminance(c)); }
float3 fl_showOverexposed(float3 c, float threshold) {
    return lerp(c, float3(1.0,0.0,0.0), step(threshold, fl_luminance(c)));
}
float3 fl_showUnderexposed(float3 c, float threshold) {
    return lerp(c, float3(0.0,0.0,1.0), 1.0 - step(threshold, fl_luminance(c)));
}
float3 fl_showGamutWarning(float3 c) {
    bool3 og = c > 1.0;
    bool3 ug = c < 0.0;
    return lerp(lerp(c, float3(1.0,0.0,0.0), og), float3(0.0,0.0,1.0), ug);
}
float3 fl_checkerboard(float2 uv, float scale) {
    float2 c = floor(uv * scale);
    return fmod(c.x + c.y, 2.0) * 0.5 + 0.25;
}

// =============================================================================
// 16. 杂项
// =============================================================================

float3 fl_premultiplyAlpha(float4 c)            { return c.rgb * c.a; }
float4 fl_unPremultiplyAlpha(float4 c) {
    if (c.a > FL_EPSILON) return float4(c.rgb / c.a, c.a);
    else                  return 0.0;
}
float3 fl_clampLDR(float3 c)                    { return fl_saturate(c); }
float fl_fxaaLuma(float3 c)                     { return c.y * (0.587/0.299) + c.x; }

float3 fl_directionalBlur(Texture2D tex, SamplerState samp, float2 uv, float2 dir, float2 texelSize, int samples) {
    float3 r = 0.0;
    for (int i = 0; i < 32; i++) {
        if (i >= samples) break;
        float t = (float(i) / float(samples - 1) - 0.5) * 2.0;
        r += tex.Sample(samp, uv + dir * t * texelSize * float(samples)).rgb;
    }
    return r / float(samples);
}

float3 fl_chromaticAberration(Texture2D tex, SamplerState samp, float2 uv, float strength) {
    float2 dir = uv - 0.5;
    float  r   = tex.Sample(samp, uv + dir * strength).x;
    float  g   = tex.Sample(samp, uv).y;
    float  b   = tex.Sample(samp, uv - dir * strength).z;
    return float3(r, g, b);
}

float fl_lensFlare(float2 uv, float2 center, float radius) {
    return fl_saturate(1.0 - length(uv - center) / radius);
}

float fl_filmGrain(float2 uv, float time, float intensity) {
    return (fl_interleavedGradientNoise(uv + time) - 0.5) * intensity;
}

float2 fl_flipHorizontal(float2 uv) { return float2(1.0-uv.x, uv.y); }
float2 fl_flipVertical(float2 uv)   { return float2(uv.x, 1.0-uv.y); }
