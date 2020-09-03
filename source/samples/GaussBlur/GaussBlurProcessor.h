#pragma once

#include <ugi/UGIDeclare.h>

namespace ugi {

    class GaussBlurTask {
    private:
        ArgumentGroup*      _argGroup;
        Texture*            _texture;
    public:
        GaussBlurTask() {
        }
        //
        Texture* target() const;
    };

    class GaussBlurProcessor {
    private:
        ugi::Pipeline*      _verticalPass;
        ugi::Pipeline*      _horizontalPass;
    public:
        GaussBlurProcessor();
    };

}