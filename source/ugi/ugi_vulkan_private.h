#pragma once
#include "ugi_types.h"
#include "vulkan_declare.h"

namespace ugi {

    struct InternalImageView {
        union {
            image_view_t _externalImageView;
            struct {
                VkImageView         _imageView;
                // Texture const*      _image;
            };
        };
        InternalImageView()
            : _externalImageView{}
        {
        }
        InternalImageView( const image_view_t& externalImageView ) {
            _externalImageView = externalImageView;
        }
        InternalImageView( const InternalImageView& internalImageView ) {
            _imageView = internalImageView._imageView;
            // _image = internalImageView._image;
        }
        InternalImageView( VkImageView imageView, Texture const* image )
            : _imageView(imageView)
            // , _image(image)
        {
        }
        VkImageView view() const {
            return _imageView;
        }
        // Texture const* texture() const {
        //     return _image;
        // }
        const image_view_t& externalImageView() const {
            return _externalImageView;
        }
        void reset() {
            _imageView = VK_NULL_HANDLE;
            // _image = VK_NULL_HANDLE;
        }
    };

    static_assert( sizeof(InternalImageView) <= sizeof(image_view_t), "" );

}
