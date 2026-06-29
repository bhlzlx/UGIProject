#pragma once
#include <UGIApplication.h>
#include <ugi/ugi_types.h>
#include "SDFFontRenderer.h"

namespace ugi {

    class TPTest : public UGIApplication {
    private:
        StandardRenderContext*  _ctx;
        SDFFontRenderer*       _fontRenderer;
        SDFFontDrawData*       _drawData;
        IndexHandle            _h1, _h2, _h3, _h4;
        float                  _width, _height;
    public:
        virtual bool initialize(void* wnd, comm::IArchive* arch);
        virtual void resize(uint32_t w, uint32_t h);
        virtual void release();
        virtual void tick();
        virtual const char* title()       { return "SDF Font Renderer"; }
        virtual uint32_t rendererType()  { return 0; }
    };
}
