#pragma once
#include "VulkanDeclare.h"
#include "UGIDeclare.h"
#include "UGITypes.h"
#include <vk_mem_alloc.h>
#include "Resource.h"

namespace ugi {

    // 纹理具备的属性
    
    class Texture : public Resource {
    private:
        TextureDescription          _description;        // descriptor
        VkImage                     _image;            // image
        VkImageView                 _imageView;        // default image view
        VmaAllocation               _allocation;        // memory allocation
        // == resource state flags ==
        VkPipelineStageFlags        _pipelineStageFlags; // pipeline stage flags
        ResourceAccessType          _primaryAccessType;  //
        ResourceAccessType          _currentAccessType;  //
        VkImageAspectFlags          _aspectFlags;        // color / depth /stencil / 
        bool                        _ownsImage;
        bool                        _ownsImageView;      // 
    private:
    public:
        VkImage image() const {
            return _image;
        }
        VkImageView imageView() const {
            return _imageView;
        }

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