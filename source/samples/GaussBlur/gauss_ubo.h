#pragma once
#include <cstdint>
#include "pipeline_ubo.h"

struct alignas(16) BlurParameter {
    float     direction[2];
    uint32_t  radius;
    uint8_t   _pad[4];    // std140: float4 需 16 字节对齐
    float     gauss[16];  // float4 gauss[4] = 16 floats
};
static_assert(sizeof(BlurParameter) >= sizeof(BlurParameter_UBO), "BlurParameter too small");
