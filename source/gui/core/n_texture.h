#pragma once
#include "core/data_types/handle.h"
#include "core/declare.h"
#include "texture.h"

namespace gui {

    class RawTexture : public ObjectHandle {
    private:
        ugi::Texture*                       tex_;
    public:
        RawTexture(ugi::Texture* tex)
            : ObjectHandle(this)
            , tex_(tex)
        {
        }
    };

    class NTexture : public ObjectHandle {
    private:
        Handle              native_;
        Handle              root_;
        Rect<float>         rc_;
        Rect<float>         uvRc_;
    public:
        NTexture() {
        }

        NTexture(RawTexture* raw, Rect<float> rc)
            : ObjectHandle(this)
            , native_(raw->handle())
            , root_(handle())
            , rc_(rc)
            , uvRc_()
        {
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
        RawTexture* nativeTexture() const {
            return native_.as<RawTexture>();
        }

        void unload();
        void reload(RawTexture* native);
    };

    static NTexture empty;

}