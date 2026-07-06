#include <UGIApplication.h>
#include <glm/glm.hpp>
#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include <io/archive.h>
#include <mesh_buffer_allocator.h>
#include <gui/render/ui_image_render.h>

#include <gui/core/package.h>
#include <gui/core/ui/stage.h>

namespace ugi {

    class FGUIDemo : public UGIApplication {
    private:
        void*                           _hwnd;                                             //
        StandardRenderContext*          _renderContext; //
        comm::IArchive*                 _arch;
        gui::UIImageRender*             _render;
        // sample resources
        float                           _width;
        float                           _height;

        gui::Stage*                      stage_;
        //
        gui::ui_render_batches_t        _imageBatches;
    public:
        virtual bool initialize(void* _wnd, comm::IArchive* arch);
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType() ;
    };

}

UGIApplication* GetApplication();