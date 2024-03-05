#pragma once
#include "core/data_types/handle.h"
#include "core/declare.h"
// #include "texture.h"

namespace gui {

    class NTexture : public ObjectHandle {
    private:
        Handle              native_;
        Handle              root_;
        Rect<float>         rc_;
        Rect<float>         uvRc_;
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