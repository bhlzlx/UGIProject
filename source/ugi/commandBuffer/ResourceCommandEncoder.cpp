#include "../VulkanFunctionDeclare.h"
#include "ResourceCommandEncoder.h"
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

    void ResourceCommandEncoder::prepareArgumentGroup( ArgumentGroup* argumentGroup ) {
        argumentGroup->prepairResource(this);
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

    void ResourceCommandEncoder::imageTransitionBarrier( Texture* texture, ResourceAccessType dstAccessType, PipelineStages srcStage, StageAccess srcStageMask, PipelineStages dstStage, StageAccess dstStageMask, const TextureSubResource* subResource) {
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
        vkCmdPipelineBarrier(
            *_commandBuffer,
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
    
    void ResourceCommandEncoder::updateImage( Texture* _dst, Buffer* _src, TextureSubResource* _dstSubRes, BufferSubResource* _srcSubRes, bool uploadMode ) {
        // Vulkan 文档里明确指出这里 dstImageLayout 必须是以下几种，但实际我们仅可能会使用 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR
        //
        if( uploadMode ) {
            imageTransitionBarrier(  _dst, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Transfer, StageAccess::Write, _dstSubRes  );
        } else {
            imageTransitionBarrier( _dst, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Top, StageAccess::Write, _dstSubRes );
        }
        bufferTransitionBarrier( _src, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Transfer, StageAccess::Read, _srcSubRes ); ///> 源 transition，无需等待，直接转换因为我们确定没有其它地方用它
        // 一个 region 只能传输一个 mip level

        std::vector< VkBufferImageCopy > regions;

        auto baseMipLevel = _dstSubRes ? _dstSubRes->baseMipLevel : 0;
        auto mipLevelCount = _dstSubRes ? _dstSubRes->mipLevelCount : _dst->desc().mipmapLevel;
        auto offsetZ = _dstSubRes ? _dstSubRes->offset.z : 0;
        auto baseArrayLayer = _dstSubRes ? _dstSubRes->baseLayer : 0;
        auto layerCount = _dstSubRes ? _dstSubRes->layerCount : _dst->desc().arrayLayers;
        auto depth = _dstSubRes? _dstSubRes->size.depth : _dst->desc().depth;
        //
        auto extWidth = _dstSubRes ? _dstSubRes->size.width : _dst->desc().width;
        auto extHeight = _dstSubRes ? _dstSubRes->size.height : _dst->desc().height;
        auto offsetX = _dstSubRes ? _dstSubRes->offset.x : 0;
        auto offsetY = _dstSubRes ? _dstSubRes->offset.y : 0;
        //
        for( uint32_t mipLevel = baseMipLevel; mipLevel< baseMipLevel + mipLevelCount; ++mipLevel) {
            VkBufferImageCopy region;
            region.imageExtent.height = extHeight;
            region.imageExtent.width = extWidth;
            region.imageExtent.depth = depth;
            region.imageOffset.x = offsetX;
            region.imageOffset.y = offsetY;
            region.imageOffset.z = offsetZ;
            //
            region.imageSubresource.baseArrayLayer = baseArrayLayer;
            region.imageSubresource.layerCount = layerCount;
            region.imageSubresource.aspectMask = _dst->aspectFlags();
            region.imageSubresource.mipLevel = mipLevel;
            region.bufferOffset = _srcSubRes->offset;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            // mip level 变小，复制的大小也应该跟着变小
            extWidth >>= 1;
            extHeight >>= 1;
            offsetX >>= 1;
            offsetY >>= 1;
            //
            regions.push_back( region );
        }
        //
        vkCmdCopyBufferToImage( *_commandBuffer, _src->buffer(), _dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data() );
        // 布局转回来
        // imageTransitionBarrier( _dst, _dst->primaryAccessType(), PipelineStages::Transfer, StageAccess::Write, PipelineStages::Bottom, StageAccess::Read, _dstSubRes );
    }

    void ResourceCommandEncoder::updateImage( Texture* dst, const uint8_t* data, const BufferImageRegionCopy* copies, uint32_t copyCount ) {

        imageTransitionBarrier(  dst, ResourceAccessType::TransferDestination, PipelineStages::Top, StageAccess::Read, PipelineStages::Transfer, StageAccess::Write, _dstSubRes  );
        // 一个 region 只能传输一个 mip level
        std::vector< VkBufferImageCopy > regions;

        auto baseMipLevel = _dstSubRes ? _dstSubRes->baseMipLevel : 0;
        auto mipLevelCount = _dstSubRes ? _dstSubRes->mipLevelCount : _dst->desc().mipmapLevel;
        auto offsetZ = _dstSubRes ? _dstSubRes->offset.z : 0;
        auto baseArrayLayer = _dstSubRes ? _dstSubRes->baseLayer : 0;
        auto layerCount = _dstSubRes ? _dstSubRes->layerCount : _dst->desc().arrayLayers;
        auto depth = _dstSubRes? _dstSubRes->size.depth : _dst->desc().depth;
        //
        auto extWidth = _dstSubRes ? _dstSubRes->size.width : _dst->desc().width;
        auto extHeight = _dstSubRes ? _dstSubRes->size.height : _dst->desc().height;
        auto offsetX = _dstSubRes ? _dstSubRes->offset.x : 0;
        auto offsetY = _dstSubRes ? _dstSubRes->offset.y : 0;
        //
        for( uint32_t mipLevel = baseMipLevel; mipLevel< baseMipLevel + mipLevelCount; ++mipLevel) {
            VkBufferImageCopy region;
            region.imageExtent.height = extHeight;
            region.imageExtent.width = extWidth;
            region.imageExtent.depth = depth;
            region.imageOffset.x = offsetX;
            region.imageOffset.y = offsetY;
            region.imageOffset.z = offsetZ;
            //
            region.imageSubresource.baseArrayLayer = baseArrayLayer;
            region.imageSubresource.layerCount = layerCount;
            region.imageSubresource.aspectMask = _dst->aspectFlags();
            region.imageSubresource.mipLevel = mipLevel;
            region.bufferOffset = _srcSubRes->offset;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            // mip level 变小，复制的大小也应该跟着变小
            extWidth >>= 1;
            extHeight >>= 1;
            offsetX >>= 1;
            offsetY >>= 1;
            //
            regions.push_back( region );
        }
        //
        vkCmdCopyBufferToImage( *_commandBuffer, _src->buffer(), _dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data() );

    }

}