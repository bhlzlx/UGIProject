#pragma once

#include "ugi_declare.h"
#include <cstdint>
#include <string.h>

namespace ugi {

    static const uint32_t MaxDescriptorNameLength = 32;
    static const uint32_t MaxRenderTarget = 4;
    static const uint32_t MaxVertexAttribute = 8;
    static const uint32_t MaxVertexBufferBinding = 8;
    static const uint32_t UniformChunkSize = 1024 * 512; // 512KB
    static const uint32_t MaxFlightCount = 2;
    static const uint32_t MaxDescriptorCount = 8;
    static const uint32_t MaxArgumentCount = 4;
    static const uint32_t MaxMaterialDescriptorCount = MaxArgumentCount * MaxDescriptorCount;
    static const uint32_t MaxNameLength = 32;
    static const uint32_t MaxMipLevelCount = 12;
    static const uint32_t MaxSubpassCount = 8;

    using GPUHandle = uint64_t;

    template <class DEST, class SOURCE>
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
        IntegerComposer& operator=(const IntegerComposer& _val)
        {
            val = _val.val;
            return *this;
        }
        bool operator==(const IntegerComposer& _val) const
        {
            return val == _val.val;
            ;
        }
        bool operator<(const IntegerComposer& _val) const
        {
            return val < _val.val;
        }
        static_assert(sizeof(DEST) == sizeof(SOURCE) * 2, "!");
    };


    template <class T>
    struct point_t {
        T x;
        T y;
    };

    template <class T>
    struct extent_2d_t {
        T width;
        T height;
    };

    template <class T>
    struct extent_3d_t {
        T width;
        T height;
        T depth;
    };

    template <class T>
    struct rect_t {
        point_t<T> origin;
        extent_2d_t<T> size;
    };

    template <class T>
    struct offset_2d_t {
        T x, y;
    };

    template <class T>
    struct offset_3d_t {
        T x, y, z;
    };

    struct viewport_t {
        float x, y;
        float width, height;
        float zNear, zFar;
    };

    typedef extent_2d_t<int> Scissor;

    // 用于操作image的某一个miplevel上的区域
    struct image_region_t {
        uint32_t mipLevel = 0;
        uint32_t arrayIndex = 0; // texture array index only avail for texture array
        uint32_t arrayCount = 1;
        offset_3d_t<int> offset = {};
        extent_3d_t<uint32_t> extent = {};
    };

    // 用于给image的某几个miplevel加资源同步操作，它没有image_region_t那么精确
    struct image_subres_t {
        uint32_t baseMipLevel;
        uint32_t mipLevelCount;
        uint32_t baseLayer;
        uint32_t layerCount;
    };

    struct buffer_subres_t {
        uint32_t offset;
        uint32_t size;
    };

    enum GraphicsAPIType {
        VULKAN = 0,
        DX12 = 1,
        METAL = 2
    };

    enum GraphicsDeviceType {
        INTEGATED = 0,
        DISCRETE = 1,
        SOFTWARE = 2,
        OTHER = 3
    };

    enum CmdbufType {
        Transient,
        Resetable,
    };

    struct device_descriptor_t {
        GraphicsAPIType apiType;
        GraphicsDeviceType deviceType;
        uint8_t debugLayer;
        uint8_t graphicsQueueCount; // for vulkan API, queue count must be specified when creating the device
        uint8_t transferQueueCount; //
        void* wnd; // surface window handle
    };

    enum class shader_stage_t : uint8_t {
        VertexShader = 0,
        TessellationControlShader,
        TessellationEvaluationShader,
        GeometryShader,
        FragmentShader,
        ComputeShader,
        ShaderStageCount,
    };

    enum class pipeline_stage_t {
        Top = 1 << 0,
        DrawIndirect = 1 << 1,
        VertexInput = 1 << 2,
        VertexShading = 1 << 3,
        TessControlShading = 1 << 4,
        TessEvaluationShading = 1 << 5,
        GeometryShading = 1 << 6,
        FragmentShading = 1 << 7,
        EaryFragmentTestShading = 1 << 8,
        LateFragmmentTestShading = 1 << 9,
        ColorAttachmentOutput = 1 << 10,
        ComputeShading = 1 << 11,
        Transfer = 1 << 12,
        Bottom = 1 << 13,
        Host = 1 << 14,
        AllGraphics = 1 << 15,
        AllCommands = 1 << 16,
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

    enum class VertexType : uint8_t {
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

    enum class topology_mode_t : uint8_t {
        Points = 0,
        LineStrip,
        LineList,
        TriangleStrip,
        TriangleList,
        TriangleFan,
    };

    enum class compare_function_t : uint8_t {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        GreaterEqual,
        Always,
    };

    enum class blend_factor_t : uint8_t {
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

    enum class blend_operation_t : uint8_t {
        Add,
        Subtract,
        Revsubtract,
    };

    enum class stencil_operation : uint8_t {
        Keep,
        Zero,
        Replace,
        IncrSat,
        DecrSat,
        Invert,
        Inc,
        Dec
    };

    enum class AddressMode : uint8_t {
        AddressModeWrap,
        AddressModeClamp,
        AddressModeMirror,
    };

    enum class TextureFilter : uint8_t {
        None,
        Point,
        Linear
    };

    enum class TextureCompareMode : uint8_t {
        RefNone = 0,
        RefToTexture = 1
    };

    enum class TextureType : uint8_t {
        Texture1D,
        Texture2D,
        Texture2DArray,
        TextureCube,
        TextureCubeArray,
        Texture3D
    };

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

    enum class AttachmentLoadAction : uint8_t {
        Keep,
        Clear,
        DontCare,
    };

    struct sampler_state_t {
        AddressMode u : 8;
        AddressMode v : 8;
        AddressMode w : 8;
        TextureFilter min : 8;
        TextureFilter mag : 8;
        TextureFilter mip : 8;
        //
        TextureCompareMode compareMode : 8;
        compare_function_t compareFunction : 8;
        bool operator<(const sampler_state_t& _ss) const
        {
            return *(uint64_t*)this < *(uint64_t*)&_ss;
        }
    };

    // 一个属性对应一个buffer,所以其stride不可能过大，这里255已经很大了
    struct vbo_desc_t {
        alignas(4) char name[MaxNameLength] = {};
        alignas(1) VertexType type = VertexType::Float;
        alignas(1) uint8_t offset = 0;
        alignas(1) uint8_t stride = 0;
        alignas(1) uint8_t instanceMode = 0;
    };

    struct vertex_layout_t {
        alignas(4) uint32_t bufferCount = 0;
        alignas(4) vbo_desc_t buffers[MaxVertexAttribute];
    };

    struct depth_state_t {
        alignas(1) uint8_t writable = 1;
        alignas(1) uint8_t testable = 1;
        alignas(1) compare_function_t cmpFunc = compare_function_t::LessEqual;
    };

    struct blend_state_t {
        alignas(1) uint8_t enable = 1;
        alignas(1) blend_factor_t srcFactor = blend_factor_t::SourceAlpha;
        alignas(1) blend_factor_t dstFactor = blend_factor_t::InvertSourceAlpha;
        alignas(1) blend_operation_t op = blend_operation_t::Add;
    };

    struct stencil_state_t {
        alignas(1) uint8_t enable = 0;
        alignas(1) stencil_operation opFail = stencil_operation::Keep;
        alignas(1) stencil_operation opZFail = stencil_operation::Keep;
        alignas(1) stencil_operation opPass = stencil_operation::Keep;
        alignas(1) uint8_t enableCCW = 0;
        alignas(1) stencil_operation opFailCCW = stencil_operation::Keep;
        alignas(1) stencil_operation opZFailCCW = stencil_operation::Keep;
        alignas(1) stencil_operation opPassCCW = stencil_operation::Keep;
        alignas(1) compare_function_t cmpFunc = compare_function_t::Greater;
        alignas(4) uint32_t mask = 0xffffffff;
    };

    enum class front_face_t : uint8_t {
        ClockWise = 0,
        CounterClockWise = 1,
    };

    enum class cull_mode_t : uint8_t {
        None = 0,
        Front = 1,
        Back = 2
    };

    enum class polygon_mode_t : uint8_t {
        Point = 0,
        Line = 1,
        Fill = 2,
    };

    struct raster_state_t {
        uint8_t depthBiasEnabled = 0;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        float depthBiasClamp = 0.0f;
        //
        front_face_t frontFace = front_face_t::ClockWise;
        cull_mode_t cullMode = cull_mode_t::None;
        polygon_mode_t polygonMode = polygon_mode_t::Fill;
    };
    //
    enum class ColorMask {
        MaskRed = 1,
        MaskGreen = 2,
        MaskBlue = 4,
        MaskAlpha = 8
    };

    struct pipeline_state_t {
        alignas(1) uint8_t writeMask = 0xff;
        alignas(1) cull_mode_t cullMode = cull_mode_t::None;
        alignas(1) front_face_t windingMode = front_face_t::ClockWise;
        alignas(1) uint8_t scissorEnable = 1;
        alignas(4) depth_state_t depthState;
        alignas(4) blend_state_t blendState;
        alignas(4) stencil_state_t stencilState;
    };

    typedef uint8_t TextureUsageFlags;

    struct tex_desc_t {
        TextureType type; // texture types
        UGIFormat format; // format
        uint32_t mipmapLevel; // mip map level count
        uint32_t layerCount; // array layer count
        //
        uint32_t width; // size
        uint32_t height;
        uint32_t depth;
    };

    // RenderPassDescription describe the
    // load action
    #pragma pack(push, 1)
    struct attachment_desc_t {
        UGIFormat format = UGIFormat::InvalidFormat;
        MultiSampleType multisample = MultiSampleType::MsaaNone;
        AttachmentLoadAction loadAction = AttachmentLoadAction::DontCare;

        ResourceAccessType initialAccessType = ResourceAccessType::ColorAttachmentReadWrite;
        ResourceAccessType finalAccessType = ResourceAccessType::ShaderRead;
        //
    };

    struct renderpass_desc_t {
        // render pass behavior
        uint32_t colorAttachmentCount = 0;
        // framebuffer description
        attachment_desc_t colorAttachments[MaxRenderTarget];
        attachment_desc_t depthStencil;
        uint32_t inputAttachmentCount = 0;
        attachment_desc_t inputAttachments[MaxRenderTarget];
        attachment_desc_t resolve;
    };

    struct optim_renderpass_desc_t {
        uint32_t subpassCount;
        renderpass_desc_t subpasses[MaxSubpassCount];
    };

    #pragma pack(pop)

    struct renderpass_clearval_t {
        struct color4 {
            float r, g, b, a;
        } colors[MaxRenderTarget];
        float depth;
        int stencil;
    };

    enum class res_descriptor_type : uint8_t {
        Sampler = 0,
        Image = 1,
        StorageImage = 2,
        StorageBuffer = 3,
        UniformBuffer = 4,
        UniformTexelBuffer = 5,
        InputAttachment = 6,
    };

    struct res_descriptor_info_t {
        alignas(4) char name[MaxNameLength] = {};
        alignas(1) uint8_t binding = 0xff;
        alignas(1) uint8_t dataSize = 0;
        alignas(1) res_descriptor_type type = res_descriptor_type::InputAttachment;
        alignas(1) shader_stage_t shaderStage = shader_stage_t::ComputeShader;
    };

    struct descriptor_set_info_t {
        alignas(4) uint8_t index = 0xff;
        alignas(4) uint8_t descriptorCount = 0;
        alignas(4) res_descriptor_info_t descriptors[MaxDescriptorCount] = {};
    };

    struct shader_desc_t {
        alignas(4) shader_stage_t type;
        union {
            alignas(8) char name[64];
            struct {
                alignas(8) uint64_t spirvData;
                alignas(8) uint32_t spirvSize;
            };
        };
        shader_desc_t()
            : type(shader_stage_t::ShaderStageCount)
            , name {}
        {
        }
    };

    constexpr uint32_t ShaderDescriptionSize = sizeof(shader_desc_t);

    struct alignas(2) pipeline_constants_t {
        alignas(1) uint8_t offset;
        alignas(1) uint8_t size;
    };

    struct pipeline_desc_t {
        // == 生成好的信息
        alignas(8) shader_desc_t shaders[(uint8_t)shader_stage_t::ShaderStageCount];
        alignas(4) uint32_t argumentCount = 0;
        alignas(4) descriptor_set_info_t argumentLayouts[MaxArgumentCount];
        alignas(4) vertex_layout_t vertexLayout;
        alignas(4) pipeline_constants_t pipelineConstants[(uint8_t)shader_stage_t::ShaderStageCount];
        alignas(4) pipeline_state_t renderState;
        alignas(1) topology_mode_t topologyMode;
        alignas(4) uint32_t tessPatchCount = 0;
    };

    // 纹理具备的属性

    // image - image view
    // 源数据和读出来的数据我们可能需要让它们不一样，比如说 通道 互换
    // 源类型可能与读取类型不一样，比如说，image是 texture2DArray, 但实际我们只读一个texture2D，这个需要指定subResource以及viewType
    enum class ChannelMapping : uint8_t {
        identity = 0,
        zero,
        one,
        red,
        green,
        blue,
        alpha
    };

    struct image_view_param_t {
        TextureType viewType;
        // =======================================
        uint32_t baseMipLevel;
        uint32_t levelCount;
        uint32_t baseArrayLayer;
        uint32_t layerCount;
        // =======================================
        ChannelMapping red;
        ChannelMapping green;
        ChannelMapping blue;
        ChannelMapping alpha;
        //
        image_view_param_t()
        {
            viewType = TextureType::Texture2D;
            baseMipLevel = 0;
            levelCount = 1;
            baseArrayLayer = 0;
            layerCount = 1;
            //
            red = ChannelMapping::identity;
            green = ChannelMapping::identity;
            blue = ChannelMapping::identity;
            alpha = ChannelMapping::identity;
        }
        //
        bool operator<(const image_view_param_t& viewParam) const
        {
            return memcmp(this, &viewParam, sizeof(image_view_param_t)) < 0;
        }
    };

    constexpr uint32_t PipelineDescriptionSize = sizeof(pipeline_desc_t);

    struct buffer_desc_t {
        size_t buffer;
        uint32_t offset;
        uint32_t size;
    };

    struct image_view_t {
        GPUHandle handle;
    };

    struct res_union_t {
        union {
            buffer_desc_t   buffer;
            GPUHandle       imageView;
            sampler_state_t samplerState;
        };
    };

    struct res_descriptor_t {
        uint32_t            handle;
        res_descriptor_type type; // 资源类型
        res_union_t         res;
        //
        res_descriptor_t()
            : handle(~0)
            , type(res_descriptor_type::UniformBuffer)
            , res {}
        {
        }
    };

    struct gpu_semaphore_t {
        GPUHandle handle;
    };

    struct gpu_fence_t {
        GPUHandle handle;
    };

}
