#include <UGIApplication.h>
#include <glm/glm.hpp>
#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include <io/archive.h>
#include <mesh_buffer_allocator.h>
#include <gui/render/ui_image_render.h>

#include <gui/core/package.h>

namespace ugi {

    class HelloWorld : public UGIApplication {
    private:
        void*                           _hwnd;                                             //
        StandardRenderContext*          _renderContext; //
        gui::UIImageRender*             _render;
        // sample resources
        ugi::Texture*                   _textures[2];
        ugi::image_view_t               _imageViews[2];
        ugi::sampler_state_t            _samplerState;                                     //
        //
        uint32_t                        _flightIndex;                                      // flight index
        //
        float                           _width;
        float                           _height;
        //
        gui::ui_render_batches_t        _imageBatches;
    public:
        virtual bool initialize(void* _wnd, comm::IArchive* arch);
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
    };

}

UGIApplication* GetApplication();