#include "Texture.h"
#include "Device.h"
#include "UGITypeMapping.h"
#include <cassert>
#include <unordered_map>
#include "UGIUtility.h"
#include <vector>
#include "resourcePool/HashObjectPool.h"

namespace ugi {

    class SamplerStateHashMethod {
    private:
    public:
        uint64_t operator() ( const SamplerState& state ) {
            UGIHash<APHash> hasher;
            hasher.hashPOD(state);
            return hasher;
        }
    };

    class SamplerCreateMethod {
    private:
    public:
        VkSampler operator()( Device* device, const SamplerState& samplerState ) {
            VkSamplerCreateInfo info; {
                info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                info.flags = 0;
                info.pNext = nullptr;
                info.addressModeU = samplerAddressModeToVk(samplerState.u);
                info.addressModeV = samplerAddressModeToVk(samplerState.v);
                info.addressModeW = samplerAddressModeToVk(samplerState.w);
                info.compareOp = compareOpToVk(samplerState.compareFunction);
                info.compareEnable = samplerState.compareMode != TextureCompareMode::RefNone;
                info.magFilter = filterToVk(samplerState.mag);
                info.minFilter = filterToVk(samplerState.min);
                info.mipmapMode = mipmapFilterToVk(samplerState.mip);
                info.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
                info.anisotropyEnable = VK_FALSE;
                info.mipLodBias = 0;
                info.maxAnisotropy = 0;
                info.minLod = 0;
                info.maxLod = 0;
                info.unnormalizedCoordinates = 0;
            }
            VkSampler sampler;
            vkCreateSampler( device->device(), &info, nullptr, &sampler);
            return sampler;
        }
    };

    class SamplerDestroyMethod {
    private:
    public:
        void operator()( Device* device, VkSampler sampler) {
            vkDestroySampler(device->device(), sampler, nullptr);
        }
    };

    using SamplerPool = HashObjectPool< SamplerState, VkSampler, Device*, SamplerStateHashMethod, SamplerCreateMethod, SamplerDestroyMethod>;

    VkSampler CreateSampler( Device* device, const SamplerState& samplerState ) {
        uint64_t hashVal = 0;
        VkSampler sampler = SamplerPool::GetInstance()->getObject( samplerState, device, hashVal );
        return sampler;
    }

    Texture* Texture::CreateTexture( Device* _device, VkImage _image, VkImageView _imageView, const TextureDescription& _desc, ResourceAccessType _accessType  ) {

        VkFormat format = UGIFormatToVk(_desc.format);
        VkImageAspectFlags aspectMask = 0;
        VkImageUsageFlags usageFlags = 0;

        bool attachment = false;

        switch( _accessType ) {
            case ResourceAccessType::ShaderRead:
                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
                usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            case ResourceAccessType::ShaderWrite:
            case ResourceAccessType::ShaderReadWrite:
            case ResourceAccessType::ColorAttachmentRead:
            case ResourceAccessType::ColorAttachmentWrite:
            case ResourceAccessType::ColorAttachmentReadWrite:
                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
                usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
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
                info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                info.queueFamilyIndexCount = _device->descriptor().queueFamilyCount;
                info.pQueueFamilyIndices = _device->descriptor().queueFamilyIndices;
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; ///> 这个值只能是两者之一： VK_IMAGE_LAYOUT_UNDEFINED / VK_IMAGE_LAYOUT_PREINITIALIZED
            }

            VmaAllocationCreateInfo allocInfo = {}; {
                allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                allocInfo.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                if( attachment ) { // 尽量给 render target 类型的做优化
                    allocInfo.memoryTypeBits |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
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
        bool ownImageView = false;
        if (!_imageView) {
            ownImageView = true;
            // create image view
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = _image;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            //
            createInfo.subresourceRange.aspectMask = aspectMask;

            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = _desc.mipmapLevel;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = _desc.depth;

            createInfo.pNext = nullptr;
            switch (_desc.type)
            {
            case TextureType::Texture2D:
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; break;
            case TextureType::Texture2DArray:
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
            case TextureType::TextureCube:
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE; break;
            case TextureType::TextureCubeArray:
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; break;
            case TextureType::Texture3D:
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_3D; break;
            case TextureType::Texture1D:
            default:
                assert(false);
            }
            createInfo.format = format;
            createInfo.flags = 0;
            // texture aspect should have all aspect information
            // but the image view should not have the stencil aspect
            createInfo.subresourceRange.aspectMask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
            VkDevice device = _device->device();
            VkResult rst = vkCreateImageView(device, &createInfo, nullptr, &_imageView);
            assert(VK_SUCCESS == rst);
        }
        //
        Texture* texture = new Texture();{
            texture->m_description = _desc;
            texture->m_image = _image;
            texture->m_imageView = _imageView;
            texture->m_allocation = allocation;
            texture->m_aspectFlags = aspectMask;
            texture->m_ownsImage = ownImage;
            texture->m_ownsImageView = ownImageView;
            // texture->m_accessFlags = 0;
            texture->m_pipelineStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            texture->m_currentAccessType = ResourceAccessType::None;
            texture->m_primaryAccessType = _accessType;
        }
        return texture;
    }

    void Texture::release( Device* _device ) {
        if( m_ownsImage && m_image ) {
            vmaDestroyImage( _device->vmaAllocator(), m_image, m_allocation );
            m_image = VK_NULL_HANDLE;
            m_ownsImage = false;
            m_allocation = nullptr;
        }
        if( m_ownsImageView && m_imageView ) {
            vkDestroyImageView( _device->device(), m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }
        delete this;
    }

    std::unordered_map<uint64_t, VkSampler> SamplerCache;

}