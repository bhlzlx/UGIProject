// PBR 公共函数库  —  GGX / Schlick / Smith / equirectangular
static const float PI = 3.141592653589793;

// GGX 法线分布 (Trowbridge-Reitz)
float D_GGX(float NoH, float roughness) {
    float a = roughness * roughness; float a2 = a * a;
    float d = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Schlick 菲涅尔近似
float3 F_Schlick(float3 F0, float VoH) {
    return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

// Smith 几何遮蔽 (GGX height-correlated)
float G_Smith(float NoV, float NoL, float roughness) {
    float k = roughness * roughness * 0.5;
    return (NoV / (NoV * (1.0 - k) + k)) * (NoL / (NoL * (1.0 - k) + k));
}

// equirectangular 采样 UV (反射方向 → 纹理坐标)
float2 envUV(float3 R) {
    float phi   = atan2(R.z, R.x);
    float theta = acos(R.y);
    return float2(phi / (2.0 * PI) + 0.5, theta / PI);
}
