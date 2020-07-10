#pragma once

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/SpvTools.h>
#include <glslang/SPIRV/Logger.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_cross/spirv_parser.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <ugi/UGITypes.h>

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV*/ 256,
    /* .maxMeshOutputPrimitivesNV*/ 512,
    /* .maxMeshWorkGroupSizeX_NV*/ 32,
    /* .maxMeshWorkGroupSizeY_NV*/ 1,
    /* .maxMeshWorkGroupSizeZ_NV*/ 1,
    /* .maxTaskWorkGroupSizeX_NV*/ 32,
    /* .maxTaskWorkGroupSizeY_NV*/ 1,
    /* .maxTaskWorkGroupSizeZ_NV*/ 1,
    /* .maxMeshViewCountNV*/ 4,
    4,
    /* .limits = */
    {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }
};

constexpr uint32_t MaxDescriptorNameLength = 64;

struct StageIOAttribute {
    uint16_t            location;
    ugi::VertexType     type;
    char                name[MaxDescriptorNameLength];
};

struct InputAttachment {
    uint16_t            set;
    uint16_t            binding;
    uint16_t            inputIndex;
    char                name[MaxDescriptorNameLength];
};

struct PushConstants {
    uint16_t            offset;
    uint16_t            size;
};

struct UniformBuffer {
    uint16_t            set;
    uint16_t            binding;
    uint32_t            size;
    char                name[MaxDescriptorNameLength];
};

struct StorageBuffer {
    uint16_t            set;
    uint16_t            binding;
    char                name[MaxDescriptorNameLength];
    size_t              size;
};

//struct TexelBufferObject {
//	uint16_t set;
//	uint16_t binding;
//	char name[MaxDescriptorNameLength];
//};

struct SeparateSampler {
    uint16_t            set;
    uint16_t            binding;
    char                name[MaxDescriptorNameLength];
};

struct SeparateImage {
    uint16_t            set;
    uint16_t            binding;
    char                name[MaxDescriptorNameLength];
};

struct StorageImage {
    uint16_t            set;
    uint16_t            binding;
    char                name[MaxDescriptorNameLength];
};

struct CombinedImageSampler {
    uint16_t            set;
    uint16_t            binding;
    char                name[MaxDescriptorNameLength];
};


ugi::VertexType getVertexType(const spirv_cross::SPIRType& _type) {
    switch (_type.basetype) {
    case spirv_cross::SPIRType::BaseType::Float: {
        ugi::VertexType types[] = {
            ugi::VertexType::Float,
            ugi::VertexType::Float,
            ugi::VertexType::Float2,
            ugi::VertexType::Float3,
        };
        return types[_type.vecsize];
    }
    case spirv_cross::SPIRType::BaseType::Int: {
        ugi::VertexType types[] = {
            ugi::VertexType::Uint,
            ugi::VertexType::Uint2,
            ugi::VertexType::Uint3,
            ugi::VertexType::Uint4,
        };
        return types[_type.vecsize];
    }
    case spirv_cross::SPIRType::BaseType::UInt: {
        ugi::VertexType types[] = {
            ugi::VertexType::Uint,
            ugi::VertexType::Uint2,
            ugi::VertexType::Uint3,
            ugi::VertexType::Uint4,
        };
        return types[_type.vecsize];
    }
    case spirv_cross::SPIRType::BaseType::UByte: {
        ugi::VertexType types[] = {
            ugi::VertexType::UByte4,
        };
        return types[_type.vecsize];
    }
    case spirv_cross::SPIRType::BaseType::Boolean: {
    }
    }
    return ugi::VertexType::Invalid;
}

uint32_t getVertexSize( ugi::VertexType type ) {
    switch( type ) {
        case ugi::VertexType::Float:{
            return 4;   
        }
		case ugi::VertexType::Float2:{
            return 8;
        }
		case ugi::VertexType::Float3:{
            return 12;
        }
		case ugi::VertexType::Float4:{
            return 16;
        }
		case ugi::VertexType::Uint:{
            return 4;
        }
		case ugi::VertexType::Uint2:{
            return 8;
        }
		case ugi::VertexType::Uint3:{
            return 12;
        }
		case ugi::VertexType::Uint4:{
            return 16;
        }
		case ugi::VertexType::Half:{
            return 2;
        }
		case ugi::VertexType::Half2:{
            return 4;
        }
		case ugi::VertexType::Half3:{
            return 6;
        }
		case ugi::VertexType::Half4:{
            return 8;
        }
		case ugi::VertexType::UByte4:{
            return 4;
        }
		case ugi::VertexType::UByte4N:{
            return 4;
        }
    }
    return 4;
}

ugi::ShaderModuleType shaderStageConvert( EShLanguage stage ) {
    switch( stage ) {
        case EShLanguage::EShLangVertex:
            return ugi::ShaderModuleType::VertexShader;
        case EShLanguage::EShLangTessControl:
            return ugi::ShaderModuleType::TessellationControlShader;
        case EShLanguage::EShLangTessEvaluation:
            return ugi::ShaderModuleType::TessellationEvaluationShader;
        case EShLanguage::EShLangGeometry:
            return ugi::ShaderModuleType::GeometryShader;
        case EShLanguage::EShLangFragment:
            return ugi::ShaderModuleType::FragmentShader;
        case EShLanguage::EShLangCompute:
            return ugi::ShaderModuleType::ComputeShader;
    }
    return ugi::ShaderModuleType::ComputeShader;
}

EShLanguage shaderStageConvertInvert( ugi::ShaderModuleType stage ) {
    switch( stage ) {
        case ugi::ShaderModuleType::VertexShader:
            return EShLanguage::EShLangVertex;
        case ugi::ShaderModuleType::TessellationControlShader :
            return EShLanguage::EShLangTessControl;
        case ugi::ShaderModuleType::TessellationEvaluationShader :
            return EShLanguage::EShLangTessEvaluation;
        case ugi::ShaderModuleType::GeometryShader:
            return EShLanguage::EShLangGeometry;
        case ugi::ShaderModuleType::FragmentShader:
            return EShLanguage::EShLangFragment;
        case ugi::ShaderModuleType::ComputeShader:
            return EShLanguage::EShLangCompute;
    }
    return EShLanguage::EShLangCompute;
}
