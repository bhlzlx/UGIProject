#include "ugi_types.h"
#include <ugi/texture.h>
#include <ugi/device.h>
#include <ugi/ugi_utility.h>
#include <ugi/ugi_type_mapping.h>
#include <ugi/resource_pool/hash_object_pool.h>
#include <ugi/helper/helper.h>
#include <command_queue.h>
#include <command_buffer.h>
#include <buffer.h>
#include <command_encoder/resource_cmd_encoder.h>
#include <asyncload/gpu_asyncload_manager.h>
#include <unordered_map>
#include <vector>
#include <cassert>

namespace ugi {

    static VkImageViewCreateInfo imageViewCreateInfo( Texture const* texture, const image_view_param_t& param ) {
        VkImageViewCreateInfo info;
        //
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.flags = 0;
        info.format = UGIFormatToVk(texture->desc().format);
        info.pNext = nullptr;
        // components
        info.components.a = (VkComponentSwizzle)param.alpha;
        info.components.r = (VkComponentSwizzle)param.red;
        info.components.g = (VkComponentSwizzle)param.green;
        info.components.b = (VkComponentSwizzle)param.blue;
        // view type
        info.viewType = imageViewType(param.viewType);
        // sub resource
        info.subresourceRange.aspectMask = texture->aspectFlags();
        info.subresourceRange.baseArrayLayer = param.baseArrayLayer;
        info.subresourceRange.layerCount = param.layerCount;
        info.subresourceRange.levelCount = param.levelCount;
        info.subresourceRange.baseMipLevel = param.baseMipLevel;
        info.image = texture->image();
        return info;
    }

    Texture* Texture::CreateTexture( Device* _device, VkImage _image, const tex_desc_t& _desc, ResourceAccessType _accessType  ) {

        VkFormat format = UGIFormatToVk(_desc.format);
        VkImageAspectFlags aspectMask = 0;
        VkImageUsageFlags usageFlags = 0;

        bool attachment = false;

        switch( _accessType ) {
            case ResourceAccessType::ShaderRead:
            case ResourceAccessType::ShaderWrite:
            case ResourceAccessType::ShaderReadWrite:

                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
                if(isCompressedFormat(format)) {
                    usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                } else {
                    usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
                }
                break;
            case ResourceAccessType::ColorAttachmentRead:
            case ResourceAccessType::ColorAttachmentWrite:
            case ResourceAccessType::ColorAttachmentReadWrite:
                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
                usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
                attachment = true;
                break;
            case ResourceAccessType::DepthStencilRead:
            case ResourceAccessType::DepthStencilWrite:
            case ResourceAccessType::DepthStencilReadWrite: {
                if ( isDepthFormat(format) ) {
                    aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT; 
                }
                if( isStencilFormat(format)) {
                    aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
                usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                attachment = true;
                break;
            }
            case ResourceAccessType::InputAttachmentRead:
            case ResourceAccessType::TransferDestination:
            case ResourceAccessType::TransferSource:
            default:
            break;
        }

        bool ownImage = false;
        VmaAllocation allocation = nullptr;
        if (!_image) {
            ownImage = true;
            VkImageCreateFlags imageFlags = 0;
            VkImageType imageType = VK_IMAGE_TYPE_1D;
            if (_desc.type == TextureType::TextureCube || _desc.type == TextureType::TextureCube ) {
                imageFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
                imageType = VK_IMAGE_TYPE_2D;
            }
            else if (_desc.type == TextureType::Texture2DArray) {
                imageType = VK_IMAGE_TYPE_2D;
            }
            else if (_desc.type == TextureType::Texture2D) {
                imageType = VK_IMAGE_TYPE_2D;
            }
            VkImageCreateInfo info; {
                info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = imageFlags;
                info.imageType = imageType;
                info.format = UGIFormatToVk(_desc.format);
                info.extent = {
                    _desc.width, _desc.height, _desc.depth
                };
                info.mipLevels = _desc.mipmapLevel;
                info.arrayLayers = _desc.layerCount;
                info.samples = VK_SAMPLE_COUNT_1_BIT;
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = usageFlags;
                if( _device->descriptor().queueFamilyCount>1 ) {
                    info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                } else {
                    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                }
                info.queueFamilyIndexCount = _device->descriptor().queueFamilyCount;
                info.pQueueFamilyIndices = _device->descriptor().queueFamilyIndices;
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; ///> 这个值只能是两者之一： VK_IMAGE_LAYOUT_UNDEFINED / VK_IMAGE_LAYOUT_PREINITIALIZED
            }

            VmaAllocationCreateInfo allocInfo = {}; {
                allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.flags = 0;
                allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                if( attachment ) { // 尽量给 render target 类型的做优化
                    allocInfo.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
                }
            };
            VkResult rst = vmaCreateImage( _device->vmaAllocator(), &info, &allocInfo, &_image, &allocation, nullptr);
            if (rst != VK_SUCCESS) {
                return nullptr;
            }
        }

        if (isDepthFormat(format)) {
            aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (isStencilFormat(format)) {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        if (!aspectMask){
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        //
        Texture* texture = new Texture();{
            texture->_description = _desc;
            texture->_image = _image;
            texture->_allocation = allocation;
            texture->_aspectFlags = aspectMask;
            texture->_ownsImage = ownImage;
            // texture->m_accessFlags = 0;
            texture->_pipelineStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            texture->_currentAccessType = ResourceAccessType::None;
            texture->_primaryAccessType = _accessType;
        }
        image_view_param_t vp;
        vp.viewType = _desc.type;
        auto iv = texture->createImageView(_device, vp);
        texture->_view = iv;
        return texture;
    }

    void Texture::release( Device* _device ) {
        if( _ownsImage && _image ) {
            vmaDestroyImage( _device->vmaAllocator(), _image, _allocation );
            _image = VK_NULL_HANDLE;
            _ownsImage = false;
            _allocation = nullptr;
        }
        delete this;
    }

    image_view_t Texture::createImageView(Device* device, const image_view_param_t& param) const {
        auto imageViewInfo = imageViewCreateInfo(this, param );
        VkImageView imageView = VK_NULL_HANDLE;
        auto rst = vkCreateImageView( device->device(), &imageViewInfo, nullptr, &imageView );
        if( rst == VK_SUCCESS) {
            InternalImageView view(imageView, this);
            return view.externalImageView();
        } else {
            return {};
        }
    }

    void Texture::destroyImageView(Device* device, image_view_t const& view) const {
        InternalImageView internalView(view);
        vkDestroyImageView(device->device(), internalView.view(), nullptr);
    }

    void Texture::updateRegions(Device* device, const image_region_t* regions, uint32_t count, uint8_t const* data, uint32_t size, uint64_t const* offsets, GPUAsyncLoadManager* asyncLoadManager, std::function<void(void*, CommandBuffer*)>&& callback) { 
        // dealing staging buffer
        Buffer* staging = device->createBuffer(BufferType::StagingBuffer, size);
        auto ptr = staging->map(device);
        memcpy(ptr, data, size);
        staging->unmap(device);
        // record command buffer
        auto &transferQueues = device->transferQueues();
        assert(transferQueues.size());
        auto queue = transferQueues[0];
        // create staging buffer
        auto cb = queue->createCommandBuffer(device, CmdbufType::Resetable);
        cb->beginEncode(); {
            auto resEnc = cb->resourceCommandEncoder();
            resEnc->imageTransitionBarrier(this, ResourceAccessType::TransferDestination, pipeline_stage_t::Top, StageAccess::Read, pipeline_stage_t::Transfer, StageAccess::Write, nullptr);
            resEnc->copyBufferToImage(this->_image, this->_aspectFlags, staging->buffer(), regions, offsets, count);
            resEnc->endEncode();
        }
        cb->endEncode();
        //
        auto fence = device->createFence(); {
            QueueSubmitInfo submitInfo(&cb, 1, nullptr, 0, nullptr, 0);
            QueueSubmitBatchInfo submitBatch(&submitInfo, 1, fence);
            queue->submitCommandBuffers(submitBatch);
        }
        auto onComplete = [](Texture* tex, Device* device, Buffer* stgbuf, CommandBuffer* transferCmd, CommandQueue* queue, std::function<void(void*,CommandBuffer*)> callback, CommandBuffer* exeBuf)->void{
            queue->destroyCommandBuffer(device, transferCmd);
            device->destroyBuffer(stgbuf);
            callback(tex, exeBuf);
        };
        using namespace std::placeholders;
        auto binder = std::bind(onComplete, this, device, staging, cb, queue, callback, _1);
        GPUAsyncLoadItem asyncLoadItem = GPUAsyncLoadItem(device, fence, std::move(binder));
        asyncLoadManager->registerAsyncLoad(std::move(asyncLoadItem));
    }

    void Texture::generateMipmap(CommandBuffer* cmdbuf) {
		auto resEncoder = cmdbuf->resourceCommandEncoder();
		// 转换为LayoutGeneral
		resEncoder->imageTransitionBarrier(this, ResourceAccessType::ShaderReadWrite, pipeline_stage_t::Bottom, StageAccess::Write, pipeline_stage_t::Top, StageAccess::Write);
		VkCommandBuffer cmdbufVk = *cmdbuf;
		VkImage image = this->image();
		std::vector<VkImageBlit> mipmapBlits;
		for( uint32_t i = 1; i<this->desc().mipmapLevel; ++i ) {
			VkImageBlit blit;
			blit.srcOffsets[0] = {};
			blit.srcOffsets[1] = { (int32_t)this->desc().width, (int32_t)this->desc().height, (int32_t)this->desc().depth };
			blit.srcSubresource.aspectMask = this->aspectFlags();
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = this->desc().layerCount;
			blit.srcSubresource.mipLevel = 0;
			//
			blit.dstOffsets[0] = {};
			blit.dstOffsets[1] = { (int32_t)this->desc().width>>i, (int32_t)this->desc().height>>i, (int32_t)this->desc().depth };
			blit.dstSubresource.aspectMask = this->aspectFlags();
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = this->desc().layerCount;
			blit.dstSubresource.mipLevel = i;
			//
			mipmapBlits.push_back(blit);
		}
		// 注意Nearest和Linear的过滤器，到时候看看效果，因为他们肯定是有效果的区别的，而且是从原图直接生成的各级mipmap
		vkCmdBlitImage( cmdbufVk, image, VK_IMAGE_LAYOUT_GENERAL, image, VK_IMAGE_LAYOUT_GENERAL, (uint32_t)mipmapBlits.size(), mipmapBlits.data(), VkFilter::VK_FILTER_NEAREST );
		resEncoder->imageTransitionBarrier(this, this->primaryAccessType(), pipeline_stage_t::Bottom, StageAccess::Write, pipeline_stage_t::Top, StageAccess::Read );
		resEncoder->endEncode();
   }

}