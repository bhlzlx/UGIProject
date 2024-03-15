#pragma once
#include "core/declare.h"
#include <ugi/texture.h>
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

        Rect<float> const& uvRc() const {
            return uvRc_;
        }

        NTexture() {
        }

        NTexture(ugi::Texture* rawTex)
            : comm::ObjectHandle(this)
            , native_(rawTex->handle())
            , root_()
            , rc_()
            , uvRc_{{0,0}, {1,1}}
            , originSize_()
            , offset_{0, 0}
        {
            auto desc = rawTex->desc();
            rc_.base = {};
            rc_.size = {(float)desc.width, (float)desc.height};
            originSize_ = rc_.size;
        }

        NTexture(Handle root, Rect<float> region, bool rotated)
            : comm::ObjectHandle(this)
            , native_()
            , root_(root)
        {
            NTexture &rootRef = *root_.as<NTexture>();
            region.base.x += rootRef.rc_.base.x;
            region.base.y += rootRef.rc_.base.y;
            uvRc_ = {
                {
                    region.base.x * rootRef.uvRc_.size.width / rootRef.rc_.size.width, 
                    region.base.y * rootRef.uvRc_.size.height / rootRef.rc_.size.height
                },
                {
                    region.size.width * rootRef.uvRc_.size.width / rootRef.rc_.size.width,
                    region.size.height * rootRef.uvRc_.size.height / rootRef.rc_.size.height
                }
            };
            if(rotated) {
                std::swap(region.size.width, region.size.height);
                std::swap(uvRc_.size.width, uvRc_.size.height);
            }
            rc_ = region;
            originSize_ = rc_.size;
        }

        NTexture(Handle root, Rect<float> region, bool rotated, Size2D<float> originSize, Point2D<float> offset)
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
            if(native_) {
                return native_;
            } else {
                auto root = root_.as<NTexture>();
                if(root) {
                    return root->nativeTexture();
                }
                return Handle();
            }
        }

        void unload();
        // void reload(RawTexture* native);
    };

    static NTexture empty;

}