#include "Texture.h"
#include "Device.h"
#include "UGITypeMapping.h"
#include <cassert>
#include <unordered_map>
#include "UGIUtility.h"
#include <vector>
#include "resourcePool/HashObjectPool.h"

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

    Texture* Texture::CreateTexture( Device* _device, VkImage _image, const TextureDescription& _desc, ResourceAccessType _accessType  ) {

        VkFormat format = UGIFormatToVk(_desc.format);
        VkImageAspectFlags aspectMask = 0;
        VkImageUsageFlags usageFlags = 0;

        bool attachment = false;

        switch( _accessType ) {
            case ResourceAccessType::ShaderRead:
            case ResourceAccessType::ShaderWrite:
            case ResourceAccessType::ShaderReadWrite:
                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
                usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
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
                info.arrayLayers = _desc.arrayLayers;
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

}