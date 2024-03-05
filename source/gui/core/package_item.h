#pragma once
#include <core/declare.h>
#include <core/hit_test.h>
#include <string>
#include "../utils/byte_buffer.h"
#include "core/ui/object_factory.h"

namespace gui {

    class PackageItem {
        friend class Package;
        friend class ObjectFactory;
    public:
        Package*                        owner_;
        PackageItemType                 type_;
        ObjectType                      objType_;
        std::string                     id_;
        std::string                     name_;
        std::string                     file_;
        int                             width_;
        int                             height_;
        ByteBuffer                      rawData_; // 有必要用指针？？？这个后续关注下！

        std::vector<std::string>*       branches_;
        std::vector<std::string>*       highResolution_;

        // if is atlas
        Texture*                        texture_;

        // if is image
        Rect<float>*                    scale9Grid_;
        int                             tileGridIndex_;
        bool                            scaledByTile_;
        PixelHitTestData*               pixelHitTestData_;

        // is is movie clip
        float                           interval_;
        float                           repeatDelay_;
        bool                            swing_;
        // Frames                       frames_; // 这个肯定是需要自己用Texture写一份了

        // component
        std::function<Component*()>     extensionCreator_;
        bool                            translated_;

        // bitmap font
        // BitmapFont*                     bitmapFont_;

        // skeleton
        Point2D<float>                  skeletonAnchor_;

    public:
        PackageItem const* getBranch() const;
        PackageItem* getBranch();
        PackageItem();
        ~PackageItem();
    };

}