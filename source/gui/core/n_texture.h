#pragma once
#include "core/declare.h"
#include <LightWeightCommon/utils/handle.h>
#include <utility>

namespace gui {

    using namespace comm;

    class NTexture : public comm::ObjectHandle {
    private:
        Handle              native_;
        Handle              root_;
        Rect<float>         rc_;
        Rect<float>         uvRc_;
        Size2D<float>       originSize_;
        Point2D<float>      offset_;
    public:
        NTexture() {
        }

        NTexture(Handle rawTex, Rect<float> rc)
            : ObjectHandle(this)
            , native_(rawTex)
            , root_(handle())
            , rc_(rc)
            , uvRc_()
        {
        }

        NTexture(NTexture root, Rect<float> region, bool rotated)
            : native_()
            , root_(root.handle())
        {
            uvRc_ = {
                {region.base.x * root.uvRc_.size.width / root.rc_.size.width, 
                1 - region.size.height + region.base.y / root.rc_.size.height},
                {region.size.width * root.uvRc_.size.width / root.rc_.size.width,
                region.size.height * root.uvRc_.size.height / root.rc_.size.height}
            };
            if(rotated) {
                std::swap(region.size.width, region.size.height);
                std::swap(uvRc_.size.width, uvRc_.size.height);
            }
            rc_ = region;
            originSize_ = rc_.size;
        }

        NTexture(NTexture root, Rect<float> region, bool rotated, Size2D<float> originSize, Point2D<float> offset)
            : NTexture(root, region, rotated)
        {
            originSize_ = originSize;
            offset_ = offset;
        }

        auto size() const {
            return rc_.size;
        }
        auto offset() const {
            return rc_.base;
        }
        NTexture* root() const {
            return root_.as<NTexture>();
        }
        Handle nativeTexture() const {
            return native_;
        }

        void unload();
        // void reload(RawTexture* native);
    };

    static NTexture empty;

}