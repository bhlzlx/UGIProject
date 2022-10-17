#pragma once
#include "VulkanDeclare.h"
#include "UGIDeclare.h"
#include "UGIVulkanPrivate.h"
#include <vk_mem_alloc.h>
#include "Resource.h"
#include <map>

namespace ugi {

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
    public:
        VkImage image() const {
            return _image;
        }

        image_view_t createImageView(Device* device, const image_view_param_t& param) const;

        void destroyImageView(Device* device, image_view_t const& view) const;

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
        static Texture* CreateTexture( Device* _device, VkImage _image, const TextureDescription& _desc, ResourceAccessType _accessType );
    };

}