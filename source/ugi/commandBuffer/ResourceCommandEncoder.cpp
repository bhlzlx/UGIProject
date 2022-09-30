#include "ResourceCommandEncoder.h"
#include "../VulkanFunctionDeclare.h"
#include "../CommandBuffer.h"
#include "../UGIDeclare.h"
#include "../RenderPass.h"
#include "../Buffer.h"
#include "../Texture.h"
#include "../Pipeline.h"
#include "../Drawable.h"
#include "../UGITypeMapping.h"
#include "../Argument.h"
#include <vector>

namespace ugi {

    VkAccessFlags GetBarrierAccessMask( PipelineStages stage, StageAccess stageTransition );

    void ResourceCommandEncoder::endEncode() {
        _commandBuffer = nullptr;
    }

    void ResourceCommandEncoder::prepareArgumentGroup( DescriptorBinder* argumentGroup ) {
        // argumentGroup->prepairResource(this);
    }

    void ResourceCommandEncoder::executionBarrier( PipelineStages _srcStage, PipelineStages _dstStage ) {
        vkCmdPipelineBarrier( 
            *_commandBuffer, 
            (VkPipelineStageFlagBits)_srcStage,     // 生产阶段
            (VkPipelineStageFlagBits)_dstStage,     // 消费阶段
            VK_DEPENDENCY_BY_REGION_BIT,            // 利用阶段关系来优化消费等待，除非你十分确定GPU执行此命令的时候资源已经完全准备好了就不用填这个了，所以我们就填这个吧
            0, nullptr,                             // 不作 transition， 用来改变memory的可访问性
            0, nullptr,                             // 不作 transition， 用来改变buffer VkAccessFlagBits
            0, nullptr                              // 不作 transition， 用来改变image layout
        );
    }



      //  Top                     = 1<<0,
      //  DrawIndirect            = 1<<1,
      //  VertexInput             = 1<<2,
      //  VertexShading           = 1<<3,
      //  TessControlShading      = 1<<4,
      //  TessEvaluationShading   = 1<<5,
      //  GeometryShading         = 1<<6,
      //  FragmentShading         = 1<<7,
      //  EaryFragmentTestShading = 1<<8,
      //  LateFragmmentTestShading= 1<<9,
      //  ColorAttachmentOutput   = 1<<10,
      //  ComputeShading          = 1<<11,
      //  Transfer                = 1<<12,
      //  Bottom                  = 1<<13,
      //  Host                    = 1<<14,
      //  AllGraphics             = 1<<15,
      //  AllCommands             = 1<<16,

    

    void ResourceCommandEncoder::bufferTransitionBarrier( Buffer* buffer, ResourceAccessType dstAccessType, PipelineStages srcStage, StageAccess srcStageMask, PipelineStages dstStage, StageAccess dstStageMask, const BufferSubResource* subResource ) {

        if( buffer->accessType() == dstAccessType ) {
            return;
        }
        VkBufferMemoryBarrier barrier; {
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.buffer = *buffer;
            barrier.srcAccessMask = GetBarrierAccessMask( srcStage, srcStageMask );
            barrier.dstAccessMask = GetBarrierAccessMask( dstStage, dstStageMask );
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.offset = subResource ?subResource->offset : 0;
            barrier.size = subResource? subResource->size : buffer->size();
        }        
        //
        vkCmdPipelineBarrier(
            *_commandBuffer,
            (VkPipelineStageFlagBits)srcStage,      // 生产阶段
            (VkPipelineStageFlagBits)dstStage,      // 消费阶段
            VK_DEPENDENCY_BY_REGION_BIT,            // 利用阶段关系来优化消费等待，除非你十分确定GPU执行此命令的时候资源已经完全准备好了就不用填这个了，所以我们就填这个吧
            0, nullptr,                             // 不作 transition， 用来改变memory的可访问性
            1, &barrier,                            // 444544444========
            0, nullptr                              // 不作 transition， 用来改变image layout
        );
        buffer->updateAccessType(dstAccessType);
    }

    void ResourceCommandEncoder::imageTransitionBarrier( Texture* texture, ResourceAccessType dstAccessType, PipelineStages srcStage, StageAccess srcStageMask, PipelineStages dstStage, StageAccess dstStageMask, const ImageSubResource* subResource) {
        if( texture->accessType() == dstAccessType ) {
            return;
        }
        //
        VkImageMemoryBarrier barrier;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = nullptr;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = GetBarrierAccessMask( srcStage, srcStageMask );
        barrier.dstAccessMask = GetBarrierAccessMask( dstStage, dstStageMask );
        barrier.oldLayout = imageLayoutFromAccessType( texture->accessType() );
        barrier.newLayout = imageLayoutFromAccessType( dstAccessType );
        barrier.subresourceRange.aspectMask = texture->aspectFlags();
        barrier.subresourceRange.baseMipLevel = subResource? subResource->baseMipLevel : 0;
        barrier.subresourceRange.levelCount = subResource ? subResource->mipLevelCount : texture->desc().mipmapLevel;
        barrier.subresourceRange.baseArrayLayer = subResource? subResource->baseLayer : 0;
        barrier.subresourceRange.layerCount = subResource? subResource->size.depth : texture->desc().depth;
        barrier.image = texture->image();
        //
        VkCommandBuffer cmdbuf = *_commandBuffer;
        vkCmdPipelineBarrier(
            cmdbuf,
            (VkPipelineStageFlagBits)srcStage,     // 生产阶段
            (VkPipelineStageFlagBits)dstStage,     // 消费阶段
            VK_DEPENDENCY_BY_REGION_BIT,            // 利用阶段关系来优化消费等待，除非你十分确定GPU执行此命令的时候资源已经完全准备好了就不用填这个了，所以我们就填这个吧
            0, nullptr,                             // 不作 transition， 用来改变memory的可访问性
            0, nullptr,                             // 不作 transition， 用来改变buffer VkAccessFlagBits
            1, &barrier
        );
        texture->updateAccessType(dstAccessType);
    }


    void ResourceCommandEncoder::updateBuffer( Buffer* _dst, Buffer* _src, BufferSubResource* _dstSubRes, BufferSubResource* _srcSubRes, bool uploadMode ) {
        VkBufferCopy region;

        VkDeviceSize dstOffset = _dstSubRes ? _dstSubRes->offset : 0;
        VkDeviceSize dstSize = _dstSubRes ? _dstSubRes->size : _dst->size();

        VkDeviceSize srcOffset = _srcSubRes ? _srcSubRes->offset : 0;
        VkDeviceSize srcSize = _srcSubRes ? _srcSubRes->size : _src->size();
        //
        assert( dstSize == srcSize );
        //
        region.dstOffset = dstOffset;
        region.srcOffset = srcOffset;
        region.size = srcSize;
        // 没错！就支持复制一块内存！
        if( uploadMode ) {
            bufferTransitionBarrier( _dst, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Transfer, StageAccess::Write, _dstSubRes  ); ///> 目标transition，适当等待一下
        } else {
            bufferTransitionBarrier( _dst, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Top, StageAccess::Write, _dstSubRes ); ///> 目标transition，适当等待一下
        }
        bufferTransitionBarrier( _src, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Transfer, StageAccess::Read, _srcSubRes); ///> 源 transition，无需等待，直接转换因为我们确定没有其它地方用它
        vkCmdCopyBuffer( *_commandBuffer, _src->buffer(), _dst->buffer(), 1, &region );
        // 万能等待
        // bufferTransitionBarrier( _dst, _dst->primaryAccessType(), PipelineStages::Transfer, StageAccess::Write, PipelineStages::Bottom, StageAccess::Read, _dstSubRes  );
        // 源buffer一般用完就回收了，可以省去转换回来的处理
    }

    void ResourceCommandEncoder::blitImage( Texture* dst, Texture* src, const ImageRegion* dstRegions, const ImageRegion* srcRegions, uint32_t regionCount ) {
        imageTransitionBarrier( src, ResourceAccessType::TransferSource, ugi::PipelineStages::Bottom, ugi::StageAccess::Write, ugi::PipelineStages::Top, ugi::StageAccess::Read );
        imageTransitionBarrier( dst, ResourceAccessType::TransferDestination, ugi::PipelineStages::Bottom, ugi::StageAccess::Write, ugi::PipelineStages::Top, ugi::StageAccess::Write );
        std::vector<VkImageBlit> regions;
        for( uint32_t i = 0; i<regionCount; ++i) {
            VkImageBlit r = {};
            r.srcSubresource.aspectMask = src->aspectFlags();
            r.srcSubresource.baseArrayLayer = 0;
            r.srcSubresource.layerCount = 1;
            r.srcSubresource.mipLevel = srcRegions->mipLevel;
            r.srcOffsets[0] = { (int32_t)srcRegions[i].offset.x, (int32_t)srcRegions[i].offset.y, (int32_t)srcRegions[i].offset.z };
            r.srcOffsets[1] = { (int32_t)srcRegions[i].extent.width + srcRegions[i].offset.x, (int32_t)srcRegions[i].extent.height + srcRegions[i].offset.y, (int32_t)srcRegions[i].extent.depth+srcRegions[i].offset.z };
            //
            r.dstSubresource.aspectMask = dst->aspectFlags();
            r.dstSubresource.baseArrayLayer = 0;
            r.dstSubresource.layerCount = 1;
            r.dstSubresource.mipLevel = dstRegions->mipLevel;
            r.dstOffsets[0] = { (int32_t)dstRegions[i].offset.x, (int32_t)dstRegions[i].offset.y, (int32_t)dstRegions[i].offset.z };
            r.dstOffsets[1] = { (int32_t)dstRegions[i].extent.width + dstRegions[i].offset.x, (int32_t)dstRegions[i].extent.height + dstRegions[i].offset.y, (int32_t)dstRegions[i].extent.depth+dstRegions[i].offset.z };
            regions.push_back(r);
        }
        vkCmdBlitImage( *_commandBuffer, src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, regions.data(), VkFilter::VK_FILTER_NEAREST );
    }

    void ResourceCommandEncoder::updateImage( Texture* dst, Buffer* src, const ImageRegion* regions, const uint32_t* offsets, uint32_t regionCount ) {
        imageTransitionBarrier(  dst, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Top, StageAccess::Write, nullptr  );
        // 一个 region 只能传输一个 mip level
        std::vector< VkBufferImageCopy > copies;

        for( uint32_t i = 0; i<regionCount; ++i) {
            const ImageRegion& region = regions[i];
            VkBufferImageCopy copy;
            copy.imageExtent.height = region.extent.height;
            copy.imageExtent.width = region.extent.width;
            copy.imageExtent.depth = region.extent.depth;
            copy.imageOffset.x = region.offset.x;
            copy.imageOffset.y = region.offset.y;
            copy.imageOffset.z = region.offset.z;
            //
            copy.imageSubresource.baseArrayLayer = region.arrayIndex;
            copy.imageSubresource.layerCount = 1;
            copy.imageSubresource.aspectMask = dst->aspectFlags();
            copy.imageSubresource.mipLevel = region.mipLevel;
            copy.bufferOffset = offsets[i];
            copy.bufferRowLength = 0;
            copy.bufferImageHeight = 0;

            copies.push_back(copy);
        }

        VkCommandBuffer cmdbuf = *_commandBuffer;
        vkCmdCopyBufferToImage( cmdbuf, src->buffer(), dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copies.size(), copies.data() );
    }

    void ResourceCommandEncoder::replaceImage( Texture* dst, Buffer* src, const ImageRegion* regions, const uint32_t* offsets, uint32_t regionCount ) {
        //
        imageTransitionBarrier(  dst, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Transfer, StageAccess::Write, nullptr );
        // 一个 region 只能传输一个 mip level
        std::vector< VkBufferImageCopy > copies;

        for( uint32_t i = 0; i<regionCount; ++i) {
            const ImageRegion& region = regions[i];
            VkBufferImageCopy copy;
            copy.imageExtent.height = region.extent.height;
            copy.imageExtent.width = region.extent.width;
            copy.imageExtent.depth = region.extent.depth;
            copy.imageOffset.x = region.offset.x;
            copy.imageOffset.y = region.offset.y;
            copy.imageOffset.z = region.offset.z;
            //
            copy.imageSubresource.baseArrayLayer = region.arrayIndex;
            copy.imageSubresource.layerCount = region.arrayCount;
            copy.imageSubresource.aspectMask = dst->aspectFlags();
            copy.imageSubresource.mipLevel = region.mipLevel;
            copy.bufferOffset = offsets[i];
            copy.bufferRowLength = 0;
            copy.bufferImageHeight = 0;

            copies.push_back(copy);
        }
        VkCommandBuffer cmdbuf = *_commandBuffer;
        vkCmdCopyBufferToImage( cmdbuf, src->buffer(), dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copies.size(), copies.data() );
    }

}