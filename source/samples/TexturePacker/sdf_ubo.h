#pragma once
#include <cstdint>
#include "pipeline_ubo.h"

struct IndexHandle {
    union {
        uint32_t handles[2];
        struct { uint32_t styleIndex:16; uint32_t transformIndex:16; uint32_t layerIndex:8; };
    };
};

// alignas: 只有含 float3/float4 等向量的 struct 才需要

struct EffectImp {
    uint32_t colorMask;
    uint32_t effectColor;
    uint32_t type;
    uint32_t padding;
};

struct TransformImp {
    float col11, col12, col13;
    float col21, col22, col23;
};

struct ContextUBO {
    float width;
    float height;
};

static_assert(sizeof(IndexHandle) * 512 <= sizeof(Indices_UBO),    "Indices UBO too small");
static_assert(sizeof(EffectImp)    * 256 <= sizeof(Effects_UBO),    "Effects UBO too small");
static_assert(sizeof(TransformImp) * 256 <= sizeof(Transforms_UBO), "Transforms UBO too small");
static_assert(sizeof(ContextUBO)        <= sizeof(Context_UBO),    "Context UBO too small");
