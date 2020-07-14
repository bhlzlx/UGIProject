#pragma once
#include "VulkanDeclare.h"
#include "UGIDeclare.h"
#include "UGITypes.h"

namespace ugi {

    class CommandBuffer;

    class ResourceCommandEncoder {
    private:
        CommandBuffer*                              _commandBuffer;
    public:
        ResourceCommandEncoder( CommandBuffer* commandBuffer = nullptr )
            : _commandBuffer( commandBuffer )
        {
        }
        ///> interface
        void prepareArgumentGroup( ArgumentGroup* argumentGroup );
        
        void executionBarrier( PipelineStages srcStage, PipelineStages dstStage);
        //// 队列里有很多指令，我目前想等待这些指令（部分执行）完，让其它阶段的的命令去处理
        //// 那么我需要尽量不干预无关的阶段执行，还要禁止在数据生产阶段没执行完就去执行处理的阶段
        void bufferTransitionBarrier( Buffer* buffer, ResourceAccessType dstAccessType, PipelineStages srcStage, StageAccess srcStageMask, PipelineStages dstStage, StageAccess dstStageMask, const BufferSubResource* subResource = nullptr );
        void imageTransitionBarrier( Texture* buffer, ResourceAccessType dstAccessType, PipelineStages srcStage, StageAccess srcStageMask, PipelineStages dstStage, StageAccess dstStageMask, const TextureSubResource* subResource = nullptr );
        void updateBuffer( Buffer* dst, Buffer* src, BufferSubResource* dstSubRes, BufferSubResource* srcSubRes, bool uploadMode = false );
        void updateImage( Texture* dst, Buffer* src, TextureSubResource* dstSubRes, BufferSubResource* srcSubRes, bool uploadMode = false );
        void endEncode();
    };

    class RenderCommandEncoder {
    private:
        CommandBuffer*                                  _commandBuffer;
        IRenderPass*                                    _renderPass;         ///> renderpass
        uint32_t                                        _subpass;            ///> subpass 的索引
        VkViewport                                      _viewport;
        VkRect2D                                        _scissor;
        ///>
        
    public:
        RenderCommandEncoder( CommandBuffer* commandBuffer = nullptr, IRenderPass* renderPass  = nullptr ) 
            : _commandBuffer( commandBuffer )
            , _renderPass( renderPass )
            , _subpass( 0 )
        {
        }
        void bindPipeline( Pipeline* pipeline );
        void bindArgumentGroup( ArgumentGroup* argumentGroup );
        void setViewport( float x, float y, float width ,float height, float minDepth, float maxDepth );
        void setScissor( int x, int y, int width, int height );
        void setLineWidth( float lineWidth );
        void draw( Drawable* drawable, uint32_t vertexCount, uint32_t baseVertexIndex);
        void drawIndexed( Drawable* drawable, uint32_t offset, uint32_t indexCount, uint32_t vertexOffset = 0 );
        void nextSubpass();
        void endEncode();
        //
        const IRenderPass* renderPass() const {
            return _renderPass;
        }
        uint32_t subpass() const {
            return _subpass;
        }
        const CommandBuffer* commandBuffer() const {
            return _commandBuffer;
        }
    };

    class CommandBuffer {
    private:
        VkCommandBuffer                                 _cmdbuff;
        VkDevice                                        _device;
        union {
            uint32_t                                    _encoderState;
            ResourceCommandEncoder                      _resourceEncoder;
            RenderCommandEncoder                        _renderPassEncoder;
        };
    private:
    public:
        CommandBuffer( VkDevice device, VkCommandBuffer cb ) 
            : _cmdbuff( cb )
            , _device(device)
            , _encoderState(0) {
        }
        //
        operator VkCommandBuffer() const;
        VkDevice device() const;

        ResourceCommandEncoder* resourceCommandEncoder();
        RenderCommandEncoder* renderCommandEncoder( IRenderPass* renderPass );
        //
        void reset();
        void beginEncode();
        void endEncode();
        //
    };

}