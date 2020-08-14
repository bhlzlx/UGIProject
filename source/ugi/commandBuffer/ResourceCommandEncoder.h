#pragma once
#include "../UGIDeclare.h"
#include "../VulkanDeclare.h"
#include "../UGITypes.h"

namespace ugi {

    struct BufferImageRegionCopy;

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
        void updateImage( Texture* dst, const uint8_t* data, const BufferImageRegionCopy* copies, uint32_t copyCount );
        void endEncode();
    };

}