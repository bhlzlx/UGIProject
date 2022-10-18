#pragma once

#include "UGITypes.h"
namespace ugi {

    static inline VkFormat UGIFormatToVk( ugi::UGIFormat _format) {
        switch (_format) {
        case UGIFormat::R8_UNORM: return VK_FORMAT_R8_UNORM;
        case UGIFormat::R16_UNORM: return VK_FORMAT_R16_UNORM;
        case UGIFormat::RGBA8888_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case UGIFormat::RGBA8888_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
        case UGIFormat::BGRA8888_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        case UGIFormat::RGB565_PACKED: return VK_FORMAT_R5G6B5_UNORM_PACK16;
        case UGIFormat::RGBA5551_PACKED: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
        case UGIFormat::RGBA_F16: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case UGIFormat::RGBA_F32: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case UGIFormat::Depth16F: return VK_FORMAT_D16_UNORM;
        case UGIFormat::Depth24FX8: return VK_FORMAT_X8_D24_UNORM_PACK32;
        case UGIFormat::Depth24FStencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case UGIFormat::Depth32F: return VK_FORMAT_D32_SFLOAT;
        case UGIFormat::Depth32FStencil8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case UGIFormat::ETC2_LINEAR_RGBA: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        case UGIFormat::EAC_RG11_UNORM: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
        case UGIFormat::BC1_LINEAR_RGBA: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case UGIFormat::BC3_LINEAR_RGBA: return VK_FORMAT_BC3_UNORM_BLOCK;
        case UGIFormat::PVRTC_LINEAR_RGBA: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
        default:
            break;
        }
        return VK_FORMAT_UNDEFINED;
    }

    static inline UGIFormat VkFormatToUGI( VkFormat _format )
    {
        switch (_format) {
        case VK_FORMAT_R8G8B8A8_UNORM: return UGIFormat::RGBA8888_UNORM;
        case VK_FORMAT_B8G8R8A8_UNORM: return UGIFormat::BGRA8888_UNORM;
        case VK_FORMAT_R8G8B8A8_SNORM: return UGIFormat::RGBA8888_SNORM;
        case VK_FORMAT_R5G6B5_UNORM_PACK16: return UGIFormat::RGB565_PACKED;
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return UGIFormat::RGBA5551_PACKED;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return UGIFormat::RGBA_F16;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return UGIFormat::RGBA_F32;
        case VK_FORMAT_D16_UNORM: return UGIFormat::Depth16F;
        case VK_FORMAT_X8_D24_UNORM_PACK32: return UGIFormat::Depth24FX8;
        case VK_FORMAT_D24_UNORM_S8_UINT: return UGIFormat::Depth24FStencil8;
        case VK_FORMAT_D32_SFLOAT: return UGIFormat::Depth32F;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return UGIFormat::Depth32FStencil8;
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return UGIFormat::ETC2_LINEAR_RGBA;
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return UGIFormat::EAC_RG11_UNORM;
        case VK_FORMAT_BC3_UNORM_BLOCK: return UGIFormat::BC3_LINEAR_RGBA;
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK : return UGIFormat::BC1_LINEAR_RGBA;
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return UGIFormat::PVRTC_LINEAR_RGBA;
        case VK_FORMAT_R8_UNORM: return UGIFormat::R8_UNORM;
        case VK_FORMAT_R16_UNORM: return UGIFormat::R16_UNORM;
        default:
            break;
        }
        return UGIFormat::InvalidFormat;
    }
    
    static inline VkBool32 isDepthFormat( VkFormat _format ) {
        switch (_format)
        {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_TRUE;
        default:
            return VK_FALSE;
        }
    }

    static inline VkBool32 isCompressedFormat(VkFormat _format)
    {
        switch (_format)
        {
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
            return VK_TRUE;
        default:
            return VK_FALSE;
        }
    }

    static inline VkBool32 isStencilFormat(VkFormat _format)
    {
        switch (_format)
        {
        case VK_FORMAT_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_TRUE;
        default:
            return VK_FALSE;
        }
    }

    static inline VkAttachmentLoadOp LoadOpToVk( ugi::AttachmentLoadAction _load) {
        switch (_load) {
        case AttachmentLoadAction::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case AttachmentLoadAction::Keep:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case AttachmentLoadAction::DontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    // static inline VkImageLayout AttachmentUsageToImageLayout(AttachmentUsageBits _usage) {
    //     switch (_usage) {
    //     case AttachmentUsageBits::ShaderRead: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //     case AttachmentUsageBits::DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //     case AttachmentUsageBits::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //     case AttachmentUsageBits::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    //     case AttachmentUsageBits::TransferSource: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    //     case AttachmentUsageBits::TransferDestination: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //     }
    //     return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // }

    static inline VkImageLayout imageLayoutFromAccessType( ResourceAccessType _type ) {
        VkImageLayout layout;
        switch( _type ) {
            case ResourceAccessType::ColorAttachmentRead:
            case ResourceAccessType::ColorAttachmentWrite: {
                layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; break;
            }
            case ResourceAccessType::DepthStencilRead:
            case ResourceAccessType::DepthStencilWrite: 
            case ResourceAccessType::DepthStencilReadWrite:{
                layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
            }
            case ResourceAccessType::Present: {
                layout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; break;
            }
            case ResourceAccessType::ShaderRead: { // sampled texture
                layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
            }
            case ResourceAccessType::ShaderReadWrite: { // storage image
                layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
                break;
            }
            case ResourceAccessType::TransferDestination: {
                layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
            }
            case ResourceAccessType::TransferSource: {
                layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
            }
            default: {
                layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED; break;
            }
        }
        return layout;
    }

    static inline ResourceAccessType accessTypeFromBufferType( BufferType _type ) {
        ResourceAccessType accessType = ResourceAccessType::None;
        switch( _type ) {
            case BufferType::IndexBuffer: {
                accessType = ResourceAccessType::IndexRead; break;
            }
            case BufferType::VertexBuffer: 
            case BufferType::VertexBufferStream: {
                accessType = ResourceAccessType::VertexAttributeRead; break;
            }
            case BufferType::ReadBackBuffer: {
                accessType = ResourceAccessType::None; break;
            }
            case BufferType::ShaderStorageBuffer: {
                accessType = ResourceAccessType::ShaderReadWrite; break;
            }
            case BufferType::StagingBuffer: {
                accessType = ResourceAccessType::TransferSource; break;
            }
            case BufferType::TexelBuffer: 
            case BufferType::UniformBuffer: { 
                accessType = ResourceAccessType::ShaderRead; break;
            }
            default:{
                break;
            }
        }
        return accessType;
    }

    static inline VkAccessFlags vkAccessFlagsFromResourceAccessType( ResourceAccessType _type ){
        VkAccessFlags flags;
        switch( _type ) {
            case ResourceAccessType::IndexRead: {
                flags = VkAccessFlagBits::VK_ACCESS_INDEX_READ_BIT; break;
            }
            case ResourceAccessType::VertexAttributeRead: {
                flags = VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT; break;
            }
            case ResourceAccessType::IndirectCommandRead: {
                flags = VkAccessFlagBits::VK_ACCESS_INDIRECT_COMMAND_READ_BIT; break;
            }
            case ResourceAccessType::InputAttachmentRead: {
                flags = VkAccessFlagBits::VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; break;
            }
            case ResourceAccessType::ShaderRead: {
                flags = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT; break;
            }
            case ResourceAccessType::ShaderWrite: {
                flags = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT; break;
            }
            case ResourceAccessType::ShaderReadWrite: {
                flags = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT | VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT; break;
            }
            case ResourceAccessType::TransferDestination: {
                flags = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT; break;
            }
            case ResourceAccessType::TransferSource: {
                flags = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT; break;
            }
            case ResourceAccessType::UniformRead: {
                flags = VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT; break;
            }
            case ResourceAccessType::ColorAttachmentRead: {
                flags = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT; break;
            }
            case ResourceAccessType::ColorAttachmentWrite: {
                flags = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; break;
            }
            case ResourceAccessType::DepthStencilRead: {
                flags = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT; break;
            }
            case ResourceAccessType::DepthStencilWrite: {
                flags = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; break;
            }
            default:
                flags = 0;
        }
        return flags;
    }

    static inline VkSamplerAddressMode samplerAddressModeToVk( AddressMode mode ) {
        switch( mode ) {
            case AddressMode::AddressModeClamp:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case AddressMode::AddressModeMirror:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case AddressMode::AddressModeWrap:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }

    static inline VkCompareOp compareOpToVk( CompareFunction _op)
	{
		switch (_op)
		{
		case CompareFunction::Never: return VK_COMPARE_OP_NEVER;
		case CompareFunction::Less: return VK_COMPARE_OP_LESS;
		case CompareFunction::Equal: return VK_COMPARE_OP_EQUAL;
		case CompareFunction::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareFunction::Greater: return VK_COMPARE_OP_GREATER;
		case CompareFunction::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareFunction::Always: return VK_COMPARE_OP_ALWAYS;
		}
		return VK_COMPARE_OP_ALWAYS;
	}

    static inline VkFilter filterToVk( TextureFilter _filter)
	{
		switch (_filter)
		{
		case TextureFilter::None:
		case TextureFilter::Point:
			return VK_FILTER_NEAREST;
		case TextureFilter::Linear:
			return VK_FILTER_LINEAR;
		}
		return VK_FILTER_NEAREST;
	}

    static inline VkSamplerMipmapMode mipmapFilterToVk( TextureFilter _filter)
	{
		switch (_filter)
		{
		case TextureFilter::None:
		case TextureFilter::Point :
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case TextureFilter::Linear:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}

    static inline VkShaderStageFlagBits shaderStageToVk( ugi::shader_type_t type ) {
        switch ( type )
        {
        case shader_type_t::VertexShader : return VK_SHADER_STAGE_VERTEX_BIT;
        case shader_type_t::TessellationControlShader : return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case shader_type_t::TessellationEvaluationShader : return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case shader_type_t::GeometryShader : return VK_SHADER_STAGE_GEOMETRY_BIT;
        case shader_type_t::FragmentShader : return VK_SHADER_STAGE_FRAGMENT_BIT;
        case shader_type_t::ComputeShader : return VK_SHADER_STAGE_VERTEX_BIT;;
            break;
        default:
            break;
        }
        return VK_SHADER_STAGE_VERTEX_BIT;
    }

    static inline VkPrimitiveTopology topolotyToVk(  ugi::topology_mode_t topoloty ) {
        switch (topoloty)
		{
		case topology_mode_t::Points: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case topology_mode_t::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case topology_mode_t::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case topology_mode_t::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case topology_mode_t::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case topology_mode_t::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			break;
		}
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }

    static inline VkBlendFactor blendFactorToVk( BlendFactor _factor)
	{
		switch (_factor)
		{
		case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
		case BlendFactor::SourceColor: return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::InvertSourceColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::SourceAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::InvertSourceAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DestinationColor: return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::InvertDestinationColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::DestinationAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor::InvertDestinationAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendFactor::SourceAlphaSat: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		}
		return VK_BLEND_FACTOR_ONE;
	}

    static inline VkBlendOp blendOpToVk( BlendOperation op ) {
        switch (op)
		{
		case BlendOperation::Add: return VK_BLEND_OP_ADD;
		case BlendOperation::Subtract: return VK_BLEND_OP_SUBTRACT;
		case BlendOperation::Revsubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		}
		return VK_BLEND_OP_ADD;
    }

    static inline VkStencilOp stencilOpToVk( StencilOperation op ) {
        switch (op)
		{
		case StencilOperation::Keep:return VK_STENCIL_OP_KEEP;
		case StencilOperation::Zero: return VK_STENCIL_OP_ZERO;
		case StencilOperation::Replace: return VK_STENCIL_OP_REPLACE;
		case StencilOperation::IncrSat:return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case StencilOperation::DecrSat:return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case StencilOperation::Invert:return VK_STENCIL_OP_INVERT;
		case StencilOperation::Inc:return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case StencilOperation::Dec:return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		}
		return VK_STENCIL_OP_REPLACE;
    }

    static inline VkFormat vertexFormatToVk( VertexType format ) {
        switch (format)
		{
		case VertexType::Float: return VK_FORMAT_R32_SFLOAT;
		case VertexType::Float2: return VK_FORMAT_R32G32_SFLOAT;
		case VertexType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
		case VertexType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		//::
		case VertexType::Uint: return VK_FORMAT_R32_UINT;
		case VertexType::Uint2: return VK_FORMAT_R32G32_UINT;
		case VertexType::Uint3: return VK_FORMAT_R32G32B32_UINT;
		case VertexType::Uint4: return VK_FORMAT_R32G32B32A32_UINT;
		//::
		case VertexType::Half: return VK_FORMAT_R16_SFLOAT;
		case VertexType::Half2: return VK_FORMAT_R16G16_SFLOAT;
		case VertexType::Half3: return VK_FORMAT_R16G16B16_SFLOAT;
		case VertexType::Half4: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case VertexType::UByte4: return VK_FORMAT_R8G8B8A8_UINT;
		case VertexType::UByte4N: return VK_FORMAT_R8G8B8A8_UNORM;
        case VertexType::Invalid: {
            break;
        }
        default:{
            break;
        }
		}
		return VK_FORMAT_UNDEFINED;
    }

    static inline VkPolygonMode polygonModeToVk( polygon_mode_t mode ) {
        switch (mode)   
        {
        case polygon_mode_t::Fill : return VK_POLYGON_MODE_FILL;
        case polygon_mode_t::Point: return VK_POLYGON_MODE_POINT;
        case polygon_mode_t::Line: return VK_POLYGON_MODE_LINE;
        default:
            break;
        }
        return VK_POLYGON_MODE_FILL;
    }

    static inline VkFrontFace frontFaceToVk( FrontFace mode ) {
        if( mode == FrontFace::ClockWise ) {
            return VK_FRONT_FACE_CLOCKWISE;
        } else {
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }
    }

    static inline VkCullModeFlags cullModeToVk( CullMode mode ) {
        switch( mode ) {
            case CullMode::None: return VK_CULL_MODE_NONE;
            case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
            case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
        }
        return VK_CULL_MODE_NONE;
    }

     static inline VkShaderStageFlags stageFlagsToVk( shader_type_t shaderStage ) {
        VkShaderStageFlags stageFlags = 0;
        switch (shaderStage)
        {
        case shader_type_t::VertexShader: {
            stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
            break;
        }
        case shader_type_t::FragmentShader: {
            stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        }
        case shader_type_t::ComputeShader: {
            stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
            break;
        }
        case shader_type_t::TessellationControlShader: {
            stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            break;
        }
        case shader_type_t::TessellationEvaluationShader: {
            stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            break;
        }
        case shader_type_t::GeometryShader: {
            stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
            break;
        }        
        default:
            break;
        }
        return stageFlags;
    }

    static inline VkDescriptorType descriptorTypeToVk( res_descriptor_type type ) {
        VkDescriptorType vkType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        switch (type)
        {
        case res_descriptor_type::Image:
            vkType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        case res_descriptor_type::UniformBuffer:
            vkType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            break;
        case res_descriptor_type::UniformTexelBuffer:
            vkType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            break;
        case res_descriptor_type::Sampler :
            vkType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case res_descriptor_type::StorageBuffer :
            vkType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        case res_descriptor_type::StorageImage:
            vkType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        case res_descriptor_type::InputAttachment:
            vkType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            break;
        default:
            break;
        }
        return vkType;
    }

}