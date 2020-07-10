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
        TextureDescription          m_description;        // descriptor
        VkImage                     m_image;            // image
        VkImageView                 m_imageView;        // default image view
        VmaAllocation               m_allocation;        // memory allocation
        // == resource state flags ==
        VkPipelineStageFlags        m_pipelineStageFlags; // pipeline stage flags
        ResourceAccessType          m_primaryAccessType;  //
        ResourceAccessType          m_currentAccessType;  //
        VkImageAspectFlags          m_aspectFlags;        // color / depth /stencil / 
        bool                        m_ownsImage;
        bool                        m_ownsImageView;      // 
    private:
    public:
        VkImage image() const {
            return m_image;
        }
        VkImageView imageView() const {
            return m_imageView;
        }

        ResourceAccessType accessType() const {
            return m_currentAccessType;
        }

        ResourceAccessType primaryAccessType() {
            return m_primaryAccessType;
        }

        void updateAccessType( ResourceAccessType _accessType ) {
            m_currentAccessType = _accessType;
        }

        const TextureDescription& desc() const {
            return m_description;
        }

        VkImageAspectFlags aspectFlags() const {
            return m_aspectFlags;
        }

        virtual void release( Device* _device ) override;
        //
        static Texture* CreateTexture( Device* _device, VkImage _image, VkImageView _imageView, const TextureDescription& _desc, ResourceAccessType _accessType );
    };


}