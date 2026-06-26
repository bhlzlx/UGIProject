#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include "pipeline_ubo.h"

namespace ugi {

// 手写 UBO — 必须显式 padding 匹配 std140 对齐规则
// float3 = 16字节对齐, float4x4 = 16字节对齐
struct alignas(16) PBRMat {
    glm::vec3 albedo;
    float     metallic;
    float     roughness;
    float     ao;
};

struct alignas(16) SceneUBO {
    glm::mat4 viewProj;
    glm::vec3 cameraPos;
};

struct alignas(16) ModelUBO {
    glm::mat4 model;
    uint32_t  materialIndex;
};

struct alignas(16) LightUBO {
    glm::vec3 lightDir;
    uint8_t   _pad1[4];    // std140: float3 后必须对齐到16字节
    glm::vec3 lightColor;
    float     ambient;
};

struct alignas(16) MaterialUBO {
    PBRMat materials[8];
};

static_assert(sizeof(SceneUBO)    >= sizeof(SceneData_UBO),    "SceneUBO too small");
static_assert(sizeof(ModelUBO)    >= sizeof(ModelData_UBO),    "ModelUBO too small");
static_assert(sizeof(LightUBO)    >= sizeof(LightData_UBO),    "LightUBO too small");
static_assert(sizeof(MaterialUBO) >= sizeof(MaterialData_UBO), "MaterialUBO too small");

} // namespace ugi
