#pragma once
#include "ugi_types.h"
#include "vulkan_declare.h"
#include "ugi_declare.h"
#include "ugi_vulkan_private.h"
#include <vk_mem_alloc.h>
#include "resource.h"
#include <map>
#include <functional>


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
        tex_desc_t                                      _description;           // descriptor
        VkImage                                         _image;                 // image
        VmaAllocation                                   _allocation;            // memory allocation
        // == resource state flags ==
        VkPipelineStageFlags                            _pipelineStageFlags;    // pipeline stage flags
        ResourceAccessType                              _primaryAccessType;     //
        ResourceAccessType                              _currentAccessType;     //
        VkImageAspectFlags                              _aspectFlags;           // color / depth /stencil / 
        bool                                            _ownsImage;
        bool                                            _uploaded;
        image_view_t                                    _view;
    public:
        VkImage image() const {
            return _image;
        }

        image_view_t defaultView() const {
            return _view;
        }

        bool uploaded() const {
            return _uploaded;
        }

        void markAsUploaded() {
            _uploaded = true;
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

        const tex_desc_t& desc() const {
            return _description;
        }

        VkImageAspectFlags aspectFlags() const {
            return _aspectFlags;
        }

        void updateRegions(
            Device* device, 
            const image_region_t* regions, uint32_t count, 
            uint8_t const* data, uint32_t dataSize, uint64_t const* offsets,
            GPUAsyncLoadManager* asnycLoadManager, 
            AsyncLoadCallback &&callback
        );

        /**
         * @brief 
         * 由于生成mipmap必须要一个graphics queue，所以
         * @param device 
         * @param asyncLoadManager 
         * @param callback 
         */
        void generateMipmap(CommandBuffer* buffer);

        virtual void release( Device* _device ) override;
        //
        static Texture* CreateTexture(Device* device, VkImage image, const tex_desc_t& desc, ResourceAccessType accessType );
    };

}