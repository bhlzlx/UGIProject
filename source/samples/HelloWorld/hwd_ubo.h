#pragma once
#include <cstdint>
#include "pipeline_ubo.h"

namespace ugi {

// Argument1: 2×4 旋转矩阵 (2 columns × float4)
struct alignas(16) ArgumentUBO {
    float col1[4];
    float col2[4];
};
static_assert(sizeof(ArgumentUBO) >= sizeof(Argument1_UBO), "ArgumentUBO too small");

} // namespace ugi
