﻿#pragma once
#include "UGITypes.h"
#include "VulkanDeclare.h"

namespace ugi {

    struct InternalImageView {
        union {
            ImageView _externalImageView;
            struct {
                VkImageView _imageView;
                Texture*    _image;
            };
        };
        InternalImageView()
            : _externalImageView{}
        {
        }
        InternalImageView( const ImageView& externalImageView ) {
            _externalImageView = externalImageView;
        }
        InternalImageView( const InternalImageView& internalImageView ) {
            _imageView = internalImageView._imageView;
            _image = internalImageView._image;
        }
        InternalImageView( VkImageView imageView, Texture* image )
            : _imageView(imageView)
            , _image(image)
        {
        }
        VkImageView view() const {
            return _imageView;
        }
        Texture* texture() const {
            return _image;
        }
    };

}