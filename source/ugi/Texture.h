#pragma once
#include "VulkanDeclare.h"
#include "UGIDeclare.h"
#include "UGITypes.h"
#include <vk_mem_alloc.h>
#include "Resource.h"
#include <map>
#include "ImageView.h"

namespace ugi {

    // 纹理具备的属性

    // image - image view
    // 源数据和读出来的数据我们可能需要让它们不一样，比如说 通道 互换
    // 源类型可能与读取类型不一样，比如说，image是 texture2DArray, 但实际我们只读一个texture2D，这个需要指定subResource以及viewType
    enum class ChannelMapping : uint8_t {
        identity = 0,
        zero,
        one,
        red,green,blue,alpha
    };

    struct ImageViewParameter {
        TextureType         viewType;
        // =======================================
        uint32_t            baseMipLevel;
        uint32_t            levelCount;
        uint32_t            baseArrayLayer;
        uint32_t            layerCount;
        // =======================================
        ChannelMapping      red;
        ChannelMapping      green;
        ChannelMapping      blue;
        ChannelMapping      alpha;
        //
        ImageViewParameter() {
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
        bool operator < ( const ImageViewParameter& viewParam ) const {
            return memcmp( this, &viewParam, sizeof(ImageViewParameter)) < 0;
        }
    };

    static VkImageViewType imageViewType( TextureType type ) {
        switch(type) {
            case TextureType::Texture1D:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_1D; break;
            case TextureType::Texture2D:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_2D; break;
            case TextureType::Texture3D:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_3D; break;
            case TextureType::Texture2DArray:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
            case TextureType::TextureCube:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE; break;
            case TextureType::TextureCubeArray:
                return VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; break;
        }
        return VkImageViewType::VK_IMAGE_VIEW_TYPE_1D;
    }
    
    class Texture : public Resource {
    private:
        TextureDescription                              _description;           // descriptor
        VkImage                                         _image;                 // image
        VmaAllocation                                   _allocation;            // memory allocation
        // == resource state flags ==
        VkPipelineStageFlags                            _pipelineStageFlags;    // pipeline stage flags
        ResourceAccessType                              _primaryAccessType;     //
        ResourceAccessType                              _currentAccessType;     //
        VkImageAspectFlags                              _aspectFlags;           // color / depth /stencil / 
        bool                                            _ownsImage;
        // image view 
        std::map<ImageViewParameter, ImageView>         _imageViews;
    private:
    public:

        VkImage image() const {
            return _image;
        }

        ImageView view( Device* device, const ImageViewParameter& param );

        ResourceAccessType accessType() const {
            return _currentAccessType;
        }

        ResourceAccessType primaryAccessType() {
            return _primaryAccessType;
        }

        void updateAccessType( ResourceAccessType _accessType ) {
            _currentAccessType = _accessType;
        }

        const TextureDescription& desc() const {
            return _description;
        }

        VkImageAspectFlags aspectFlags() const {
            return _aspectFlags;
        }

        virtual void release( Device* _device ) override;
        //
        static Texture* CreateTexture( Device* _device, VkImage _image, VkImageView _imageView, const TextureDescription& _desc, ResourceAccessType _accessType );
    };

}