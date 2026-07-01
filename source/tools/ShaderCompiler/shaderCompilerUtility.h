#pragma once

#include <spirv_reflect.h>
#include "UGITypes.h"

// SPIRV-Reflect 版本的 vertex type 推断
inline ugi::VertexType getVertexType(const SpvReflectInterfaceVariable& iv) {
    uint32_t compCount = iv.numeric.vector.component_count;
    if (compCount == 0) compCount = 1;
    uint32_t width = iv.numeric.scalar.width;
    uint32_t flags = iv.type_description ? iv.type_description->type_flags : 0;

    if (flags & SPV_REFLECT_TYPE_FLAG_INT) {
        if (width == 8) {
            return compCount == 4 ? ugi::VertexType::UByte4 : ugi::VertexType::Invalid;
        }
        ugi::VertexType types[] = {
            ugi::VertexType::Uint, ugi::VertexType::Uint,
            ugi::VertexType::Uint2, ugi::VertexType::Uint3, ugi::VertexType::Uint4,
        };
        return compCount <= 4 ? types[compCount] : ugi::VertexType::Invalid;
    }
    if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
        ugi::VertexType types[] = {
            ugi::VertexType::Float, ugi::VertexType::Float,
            ugi::VertexType::Float2, ugi::VertexType::Float3, ugi::VertexType::Float4,
        };
        return compCount <= 4 ? types[compCount] : ugi::VertexType::Invalid;
    }
    return ugi::VertexType::Invalid;
}

inline uint32_t getVertexSize( ugi::VertexType type ) {
    switch( type ) {
        case ugi::VertexType::Float:   return 4;
        case ugi::VertexType::Float2:  return 8;
        case ugi::VertexType::Float3:  return 12;
        case ugi::VertexType::Float4:  return 16;
        case ugi::VertexType::Uint:    return 4;
        case ugi::VertexType::Uint2:   return 8;
        case ugi::VertexType::Uint3:   return 12;
        case ugi::VertexType::Uint4:   return 16;
        case ugi::VertexType::Half:    return 2;
        case ugi::VertexType::Half2:   return 4;
        case ugi::VertexType::Half3:   return 6;
        case ugi::VertexType::Half4:   return 8;
        case ugi::VertexType::UByte4:  return 4;
        case ugi::VertexType::UByte4N: return 4;
        default: return 4;
    }
}
