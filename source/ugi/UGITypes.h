#pragma once

#include <cstdint>
#include "UGIDeclare.h"

namespace ugi {

    static const uint32_t MaxDescriptorNameLength   = 32;
    static const uint32_t MaxRenderTarget           = 4;
    static const uint32_t MaxVertexAttribute        = 8;
    static const uint32_t MaxVertexBufferBinding    = 8;
    static const uint32_t UniformChunkSize          = 1024 * 512; // 512KB
    static const uint32_t MaxFlightCount            = 2;
    static const uint32_t MaxDescriptorCount        = 8;
    static const uint32_t MaxArgumentCount          = 4;
    static const uint32_t MaxNameLength             = 32;
    static const uint32_t MaxMipLevelCount          = 12;
    static const uint32_t MaxSubpassCount           = 8;

    template< class DEST, class SOURCE >
    struct IntegerComposer {
        union {
            struct {
                SOURCE a;
                SOURCE b;
            };
            DEST val;
        };
        IntegerComposer(SOURCE _a, SOURCE _b)
            : a(_a)
            , b(_b)
        {
        }
        IntegerComposer& operator=(const IntegerComposer& _val) {
            val = _val.val;
            return *this;
        }
        bool operator == (const IntegerComposer& _val) const {
            return val == _val.val;;
        }
        bool operator < (const IntegerComposer& _val) const {
            return val < _val.val;
        }
        static_assert(sizeof(DEST) == sizeof(SOURCE) * 2, "!");
    };

    template < class T >
    struct Point {
        T x;
        T y;
    };

    template < class T >
    struct Size {
        T width;
        T height;
    };

    template < class T >
    struct Size3D {
        T width;
        T height;
        T depth;
    };

    template< class T >
    struct Rect {
        Point<T> origin;
        Size<T> size;
    };

    template< class T >
    struct Offset2D {
        T x, y;
    };

    template< class T >
    struct Offset3D {
        T x, y, z;
    };

    struct Viewport {
        float x, y;
        float width, height;
        float zNear, zFar;
    };

    typedef Rect<int> Scissor;

    // 仅能表达某一级mipmap
    struct TextureRegion {
        uint32_t mipLevel; // mip map level
        // for texture 2d, baseLayer must be 0
        // for texture 2d array, baseLayer is the index
        // for texture cube, baseLayer is from 0 ~ 5
        // for texture cube array, baseLayer is ( index * 6 + face )
        // for texture 3d, baseLayer is ( depth the destination layer )
        // for texture 3d array, baseLayer is ( {texture depth} * index + depth of the destination layer )
        uint32_t baseLayer;
        // for texture 2d, offset.z must be 0
        Offset3D<uint32_t> offset;
        // for texture 2d, size.depth must be 1
        Size3D<uint32_t> size;
    };

    struct TextureSubResource {
        uint32_t            baseMipLevel;
        uint32_t            mipLevelCount;
        uint32_t            baseLayer;
        uint32_t            layerCount;
        Offset3D<uint32_t>  offset;
        Size3D<uint32_t>    size;
    };

    struct BufferSubResource {
        uint32_t            offset;
        uint32_t            size;
    };

    enum GRAPHICS_API_TYPE {
        VULKAN          = 0,
        DX12            = 1,
        METAL           = 2
    };

    enum GRAPHICS_DEVICE_TYPE {
        INTEGATED       = 0,
        DISCRETE        = 1,
        SOFTWARE        = 2,
        OTHER           = 3
    };

    struct DeviceDescriptor {
        GRAPHICS_API_TYPE           apiType;
        GRAPHICS_DEVICE_TYPE        deviceType;
        uint8_t                     debugLayer;
        uint8_t                     graphicsQueueCount;        // for vulkan API, queue count must be specified when creating the device
        uint8_t                     transferQueueCount;        // 
        void*                       wnd;                    // surface window handle
    };

    enum class ShaderModuleType : uint8_t {
        VertexShader = 0,
        TessellationControlShader,
        TessellationEvaluationShader,
        GeometryShader,
        FragmentShader,
        ComputeShader,
        ShaderTypeCount,
    };

    enum class PipelineStages {
        Top                     = 1<<0,
        DrawIndirect            = 1<<1,
        VertexInput             = 1<<2,
        VertexShading           = 1<<3,
        TessControlShading      = 1<<4,
        TessEvaluationShading   = 1<<5,
        GeometryShading         = 1<<6,
        FragmentShading         = 1<<7,
        EaryFragmentTestShading = 1<<8,
        LateFragmmentTestShading= 1<<9,
        ColorAttachmentOutput   = 1<<10,
        ComputeShading          = 1<<11,
        Transfer                = 1<<12,
        Bottom                  = 1<<13,
        Host                    = 1<<14,
        AllGraphics             = 1<<15,
        AllCommands             = 1<<16,
        // 其实后边还有很多没写的
    };

    enum class UGIFormat : uint8_t {
        InvalidFormat,
        //
        R8_UNORM,
        R16_UNORM,
        // 32 bit
        RGBA8888_UNORM,
        BGRA8888_UNORM,
        RGBA8888_SNORM,
        // 16 bit
        RGB565_PACKED,
        RGBA5551_PACKED,
        // Float type
        RGBA_F16,
        RGBA_F32,
        //
        Depth16F,
        Depth24FX8,
        // Depth stencil type
        Depth24FStencil8,
        Depth32F,
        Depth32FStencil8,
        // compressed type
        ETC2_LINEAR_RGBA,
        EAC_RG11_UNORM,
        BC1_LINEAR_RGBA,
        BC3_LINEAR_RGBA,
        PVRTC_LINEAR_RGBA,
    };

    enum class VertexType :uint8_t {
		Float,
		Float2,
		Float3,
		Float4,
		Uint,
		Uint2,
		Uint3,
		Uint4,
		Half,
		Half2,
		Half3,
		Half4,
		UByte4,
		UByte4N,
		Invalid
	};

    enum class TopologyMode :uint8_t {
        Points = 0,
        LineStrip,
        LineList,
        TriangleStrip,
        TriangleList,
        TriangleFan,
    };

    enum class CompareFunction : uint8_t {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        GreaterEqual,
        Always,
    };

    enum class BlendFactor :uint8_t {
        Zero,
        One,
        SourceColor,
        InvertSourceColor,
        SourceAlpha,
        InvertSourceAlpha,
        SourceAlphaSat,
        //
        DestinationColor,
        InvertDestinationColor,
        DestinationAlpha,
        InvertDestinationAlpha,
    };

    enum class BlendOperation :uint8_t {
        Add,
        Subtract,
        Revsubtract,
    };

    enum class StencilOperation :uint8_t {
        Keep,
        Zero,
        Replace,
        IncrSat,
        DecrSat,
        Invert,
        Inc,
        Dec
    };

    enum class AddressMode :uint8_t {
        AddressModeWrap,
        AddressModeClamp,
        AddressModeMirror,
    };

    enum class TextureFilter :uint8_t {
        None,
        Point,
        Linear
    };

    enum class TextureCompareMode :uint8_t {
        RefNone = 0,
        RefToTexture = 1
    };

    enum class TextureType :uint8_t {
        Texture1D,
        Texture2D,
        Texture2DArray,
        TextureCube,
        TextureCubeArray,
        Texture3D
    };

    // enum class TextureUsageFlagBits :uint8_t {
    //     TextureUsageNone = 0x0,
    //     TextureUsageTransferSource = 0x1,
    //     TextureUsageTransferDestination = 0x2,
    //     TextureUsageSampled = 0x4,
    //     TextureUsageStorage = 0x8,
    //     TextureUsageColorAttachment = 0x10,
    //     TextureUsageDepthStencilAttachment = 0x20,
    //     TextureUsageTransientAttachment = 0x40,
    //     TextureUsageInputAttachment = 0x80
    // };

    // enum class AttachmentUsageBits : uint8_t {
    //     ColorAttachment,
    //     DepthStencilAttachment,
    //     ShaderRead,
    //     TransferSource,
    //     TransferDestination,
    //     Present,
    // };

    enum class BufferType : uint8_t {
        VertexBuffer,
        VertexBufferStream,
        IndexBuffer,
        ShaderStorageBuffer,
        TexelBuffer,
        UniformBuffer,
        StagingBuffer,
        ReadBackBuffer,
        InvalidBuffer
    };

    enum class ResourceAccessType : uint8_t {
        None,
        // buffers
        IndirectCommandRead,
        IndexRead,
        VertexAttributeRead,
        UniformRead,
        // images
        InputAttachmentRead,
        ColorAttachmentRead,
        ColorAttachmentWrite,
        ColorAttachmentReadWrite,
        DepthStencilRead,
        DepthStencilWrite,
        DepthStencilReadWrite,
        Present,
        // buffer & image
        TransferSource,
        TransferDestination,
        ShaderRead,
        ShaderWrite,
        ShaderReadWrite,
        //
        Prototype,
        // 其它的暂时不支持了
    };

    enum class StageAccess : uint8_t {
        Read,
        Write,
    };

    enum class MultiSampleType {
        MsaaNone,
        Msaax2,
        Msaax4,
        Msaax8,
        Msaax16
    };

    enum class AttachmentLoadAction :uint8_t {
        Keep,
        Clear,
        DontCare,
    };

    struct SamplerState {
        AddressMode u : 8;
        AddressMode v : 8;
        AddressMode w : 8;
        TextureFilter min : 8;
        TextureFilter mag : 8;
        TextureFilter mip : 8;
        //
        TextureCompareMode compareMode : 8;
        CompareFunction compareFunction : 8;
        bool operator < (const SamplerState& _ss) const {
            return *(uint64_t*)this < *(uint64_t*)&_ss;
        }
    };

	// 一个属性对应一个buffer,所以其stride不可能过大，这里255已经很大了
    struct VertexAttribueDescription {
        alignas(4) char			name[MaxNameLength];
        alignas(1) VertexType	type;
		alignas(1) uint8_t		stride;
		alignas(1) uint8_t		instanceMode;
        VertexAttribueDescription()
			: name {}
			, type(VertexType::Float)
			, instanceMode(0)
		{
        }
    };

    struct VertexBufferDescription {
        alignas(4) uint32_t stride;
        alignas(4) uint32_t instanceMode;
        VertexBufferDescription() :
            stride(0),
            instanceMode(0)
        {
        }
    };

    struct VertexLayout {
        alignas(4) uint32_t         attributeCount;
        VertexAttribueDescription   attributes[MaxVertexAttribute];
        uint32_t                    bufferCount;
        VertexBufferDescription     buffers[MaxVertexBufferBinding];
        VertexLayout() : attributeCount(0), bufferCount(0) {
        }
    };

    struct DepthState {
        alignas(1) uint8_t             writable = 1;
        alignas(1) uint8_t             testable = 1;
        alignas(1) CompareFunction     cmpFunc = CompareFunction::LessEqual;
    };

    struct BlendState {
        alignas(1) uint8_t             enable = 1;
        alignas(1) BlendFactor         srcFactor = BlendFactor::SourceAlpha;
        alignas(1) BlendFactor         dstFactor = BlendFactor::InvertSourceAlpha;
        alignas(1) BlendOperation      op = BlendOperation::Add;
    };

    struct StencilState {
        alignas(1) uint8_t             enable = 0;
        alignas(1) StencilOperation    opFail = StencilOperation::Keep;
        alignas(1) StencilOperation    opZFail = StencilOperation::Keep;
        alignas(1) StencilOperation    opPass = StencilOperation::Keep;
        alignas(1) uint8_t             enableCCW = 0;
        alignas(1) StencilOperation    opFailCCW = StencilOperation::Keep;
        alignas(1) StencilOperation    opZFailCCW = StencilOperation::Keep;
        alignas(1) StencilOperation    opPassCCW = StencilOperation::Keep;
        alignas(1) CompareFunction     cmpFunc = CompareFunction::Greater;
        alignas(4) uint32_t mask = 0xffffffff;
    };

    enum class FrontFace : uint8_t {
        ClockWise = 0,
        CounterClockWise = 1,
    };

    enum class CullMode : uint8_t {
        None = 0,
        Front = 1,
        Back = 2
    };

    enum class PolygonMode : uint8_t {
        Point = 0,
        Line = 1,
        Fill = 2,
    };

    struct RasterizationState {
        uint8_t         depthBiasEnabled = 0;
        float           depthBiasConstantFactor = 0.0f;
        float           depthBiasSlopeFactor = 0.0f;
        float           depthBiasClamp = 0.0f;
        //
        FrontFace       frontFace = FrontFace::ClockWise;
        CullMode        cullMode = CullMode::None;
        PolygonMode     polygonMode = PolygonMode::Fill;
    };
    //
    enum class ColorMask {
        MaskRed = 1,
        MaskGreen = 2,
        MaskBlue = 4,
        MaskAlpha = 8
    };

    struct PipelineState {
        alignas(1) uint8_t                  writeMask = 0xf;
        alignas(1) CullMode                 cullMode = CullMode::None;
        alignas(1) FrontFace                windingMode = FrontFace::ClockWise;
        alignas(1) uint8_t                  scissorEnable = 1;
        alignas(4) DepthState               depthState;
        alignas(4) BlendState               blendState;
        alignas(4) StencilState             stencilState;
    };

    typedef uint8_t TextureUsageFlags;

    struct TextureDescription {
        TextureType     type;                   // texture types
        UGIFormat       format;                 // format
        uint32_t        mipmapLevel;            // mip map level count
        uint32_t        arrayLayers;            // array layer count
        //
        uint32_t        width;                  // size
        uint32_t        height;
        uint32_t        depth;
    };

    // RenderPassDescription describe the 
    // load action
#pragma pack( push, 1 )
    struct AttachmentDescription {
        UGIFormat                           format = UGIFormat::InvalidFormat;
        MultiSampleType                     multisample = MultiSampleType::MsaaNone;
        AttachmentLoadAction                loadAction = AttachmentLoadAction::DontCare;

        ResourceAccessType                  initialAccessType = ResourceAccessType::ColorAttachmentReadWrite;
        ResourceAccessType                  finalAccessType = ResourceAccessType::ShaderRead;
        //
    };
    
    struct RenderPassDescription {
        // render pass behavior
        uint32_t                        colorAttachmentCount = 0;
        // framebuffer description
        AttachmentDescription           colorAttachments[MaxRenderTarget];
        AttachmentDescription           depthStencil;
        uint32_t                        inputAttachmentCount = 0;
        AttachmentDescription           inputAttachments[MaxRenderTarget];
        AttachmentDescription           resolve;

    };

    struct OptimizableSequenceRenderPassDescription {
        uint32_t                     subpassCount;
        RenderPassDescription         subpasses[MaxSubpassCount];
    };

#pragma pack (pop)


    struct RenderPassClearValues {
        struct color4 {
            float r, g, b, a;
        } colors[MaxRenderTarget];
        float depth;
        int stencil;
    };

    enum class ArgumentDescriptorType : uint8_t {
        Sampler                 = 0,
        Image                   = 1,
        StorageImage            = 2,
        StorageBuffer           = 3,
        UniformBuffer           = 4,
        UniformTexelBuffer      = 5,
        InputAttachment         = 6,
    };

    struct ArgumentDescriptorInfo {
        alignas(4) char         name[MaxNameLength] = {};
        alignas(1) uint8_t      binding = 0xff;
        alignas(1) uint8_t      dataSize = 0;
        alignas(1) ArgumentDescriptorType  type = ArgumentDescriptorType::InputAttachment;
        alignas(1) ShaderModuleType        shaderStage = ShaderModuleType::ComputeShader;
    };

    struct ArgumentInfo {
        alignas(4) uint8_t                          index = 0xff;
        alignas(4) uint8_t                          descriptorCount = 0;
        alignas(4) ArgumentDescriptorInfo           descriptors[MaxDescriptorCount] = {};
    };

    struct ShaderDescription {
        alignas(4) ShaderModuleType             type;
        union {
            alignas(8) char                     name[64];
            struct {
                alignas(8) uint64_t             spirvData;
                alignas(8) uint32_t             spirvSize;
            };
        };
        ShaderDescription()
            : type(ShaderModuleType::ShaderTypeCount)
            , name {}
        {
            
        }
    };

    constexpr uint32_t ShaderDescriptionSize = sizeof( ShaderDescription );

    struct alignas(2) PipelineConstants {
        alignas(1) uint8_t offset;
        alignas(1) uint8_t size;
    };

    struct PipelineDescription {
        // == 生成好的信息
        alignas(8) ShaderDescription                shaders[(uint8_t)ShaderModuleType::ShaderTypeCount];
        alignas(4) PipelineConstants                pipelineConstants[(uint8_t)ShaderModuleType::ShaderTypeCount];
        alignas(4) uint32_t                         argumentCount = 0;
        alignas(4) ArgumentInfo                     argumentLayouts[MaxArgumentCount];
        alignas(4) VertexLayout                     vertexLayout;
        // ==
        alignas(4) PipelineState                    renderState;
        alignas(4) uint32_t                         tessPatchCount = 0;
        alignas(1) TopologyMode                     topologyMode;
        alignas(1) PolygonMode                      pologonMode;
    };

    constexpr uint32_t PipelineDescriptionSize = sizeof(PipelineDescription);

    struct ResourceDescriptor {
        uint32_t                descriptorHandle;
        ArgumentDescriptorType  type;                   // 资源类型
        union {
            Buffer*       buffer;
            Texture*            texture;
            SamplerState        sampler;
        };
        union {
            struct {
                uint32_t            bufferOffset;
                uint32_t            bufferRange;
            };
        };
        //
        ResourceDescriptor()
            : descriptorHandle(~0)
            , type( ArgumentDescriptorType::UniformBuffer )
            , buffer( nullptr ) {
        }
    };

    struct BufferImageUpload {
        const void*                         data;
        uint32_t                            length;
        TextureRegion                       baseMipRegion;
        uint32_t                            mipCount;
        uint64_t                            mipDataOffsets[MaxMipLevelCount];
    };

}

