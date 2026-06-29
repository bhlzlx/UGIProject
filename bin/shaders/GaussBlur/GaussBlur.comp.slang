// GaussBlur compute shader — Slang version
[[vk::binding(0, 0)]] Texture2D<float4> InputImage;
[[vk::binding(1, 0)]] RWTexture2D<float4> OutputImage;

[[vk::binding(2, 0)]] cbuffer BlurParameter {
    float2 dir;
    uint   radius;
    float4 gauss[4];
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 tc = id.xy;
    float4 origColor = InputImage[tc] * gauss[0].x;
    float4 composedColor = origColor;

    for (int i = 1; i < radius; i++) {
        uint idx0 = i / 4u;
        uint idx1 = i % 4u;
        float ratio = gauss[idx0][idx1];
        float4 cr = InputImage[int2(float2(tc) + dir * float(i))];
        float4 cl = InputImage[int2(float2(tc) - dir * float(i))];

        if (cr.a < 0.001)
            composedColor += cl * (ratio * 2.0);
        else if (cl.a < 0.001)
            composedColor += cr * (ratio * 2.0);
        else
            composedColor += (cr + cl) * ratio;
    }

    OutputImage[tc] = composedColor;
}
