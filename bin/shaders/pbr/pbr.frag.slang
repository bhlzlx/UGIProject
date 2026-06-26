// PBR — Blinn-Phong + optional env map
import pbr_lib;

struct FSInput {
    [[vk::location(0)]] float3 worldPos  : TEXCOORD0;
    [[vk::location(1)]] float3 worldNorm : TEXCOORD1;
    [[vk::location(2)]] float2 uv        : TEXCOORD2;
    [[vk::location(3)]] uint   matIndex  : TEXCOORD3;
    float4 position : SV_Position;
};

struct PBRMat { float3 albedo; float metallic; float roughness; float ao; };

[[vk::binding(0, 0)]] cbuffer SceneData    { float4x4 viewProj; float3 cameraPos; }
[[vk::binding(2, 0)]] cbuffer LightData    { float3 lightDir; float3 lightColor; float ambient; }
[[vk::binding(3, 0)]] cbuffer MaterialData { PBRMat materials[8]; }
[[vk::binding(4, 0)]] SamplerState         envSampler;
[[vk::binding(5, 0)]] Texture2D<float4>    envTex;

[shader("fragment")]
float4 main(FSInput input) : SV_Target
{
    float3 N = normalize(input.worldNorm);
    float3 V = normalize(cameraPos - input.worldPos);
    float3 L = normalize(lightDir);
    float3 H = normalize(V + L);

    PBRMat m = materials[input.matIndex];
    float  r = max(m.roughness, 0.05);

    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);

    float3 diff = m.albedo * NoL * (1.0 - m.metallic);

    float specPower = lerp(256.0, 4.0, r);
    float spec      = pow(NoH, specPower);
    float3 F0       = lerp(0.04, m.albedo, m.metallic);
    float3 specC    = (F0 + (1.0 - F0) * spec) * spec;

    float3 color = (diff + specC) * lightColor;
    color += m.albedo * ambient * m.ao;

    // 环境反射 (金属用)
    float3 R = reflect(-V, N);
    float3 env = envTex.Sample(envSampler, envUV(R)).rgb;
    color += env * m.metallic * lerp(0.5, 0.15, r);

    return float4(color, 1.0);
}
