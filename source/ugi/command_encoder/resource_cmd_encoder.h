#pragma once
#include <ugi/ugi_declare.h>
#include <ugi/vulkan_declare.h>
#include <ugi/ugi_types.h>

namespace ugi {

    struct image_region_t;

    class ResourceCommandEncoder {
    private:
        CommandBuffer*                              _commandBuffer;
    public:
        ResourceCommandEncoder( CommandBuffer* commandBuffer = nullptr )
            : _commandBuffer( commandBuffer )
        {
        }
        
        void executionBarrier(PipelineStages srcStage, PipelineStages dstStage);
        //// 队列里有很多指令，我目前想等待这些指令（部分执行）完，让其它阶段的的命令去处理
        //// 那么我需要尽量不干预无关的阶段执行，还要禁止在数据生产阶段没执行完就去执行处理的阶段
        void bufferBarrier(VkBuffer buff, ResourceAccessType dstAccessType, PipelineStages srcStage, StageAccess srcStageMask, PipelineStages dstStage, StageAccess dstStageMask, const buffer_subres_t& subResource );
        void bufferTransitionBarrier(Buffer* buffer, ResourceAccessType dstAccessType, PipelineStages srcStage, StageAccess srcStageMask, PipelineStages dstStage, StageAccess dstStageMask, const buffer_subres_t* subResource = nullptr );
        void imageTransitionBarrier(Texture* buffer, ResourceAccessType dstAccessType, PipelineStages srcStage, StageAccess srcStageMask, PipelineStages dstStage, StageAccess dstStageMask, const image_subres_t* subResource = nullptr );
        void updateBuffer(Buffer* dst, Buffer* src, buffer_subres_t* dstSubRes, buffer_subres_t* srcSubRes, bool uploadMode = false );
        // void blitBuffer(VkBuffer dst, VkBuffer src, buffer_subres_t dstRange, buffer_subres_t srcRange);
        // update texture region for specified regions( render command queue )
        void updateImage( Texture* dst, Buffer* src, const image_region_t* regions, const uint32_t* offsets, uint32_t regionCount );
        // 
        void blitImage(Texture* dst, Texture* src, const image_region_t* dstRegions, const image_region_t* srcRegions, uint32_t regionCount );
        // update texture region for specified regions( transfer queue )
        void replaceImage( Texture* dst, Buffer* src, const image_region_t* regions, const uint32_t* offsets, uint32_t regionCount );

        // latest interface
        void copyBuffer(VkBuffer dst, VkBuffer src, buffer_subres_t dstRange, buffer_subres_t srcRange);
        void copyBufferToImage(VkImage dst, VkImageAspectFlags aspectFlags, VkBuffer src, const image_region_t* regions, const uint64_t* offsets, uint32_t regionCount);
        //
        void endEncode();
    };

}