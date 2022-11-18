#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <ugi/Argument.h>
#include <io/archive.h>

namespace ugi {

    class Render {
    private:
        ugi::GraphicsPipeline*          _pipeline;
        MeshBufferAllocator*            _bufferAllocator;
        UniformAllocator*               _uniformAllocator;
        Device*                         _device;
        res_descriptor_t                _uboptor;                                       // matrices
        res_descriptor_t                _texptor;
        res_descriptor_t                _samptor;
    public:
        Render(Device* device, ugi::GraphicsPipeline* pipeline, ugi::MeshBufferAllocator* msalloc, UniformAllocator* uniformAllocator)
            : _pipeline(pipeline)
            , _bufferAllocator(msalloc)
            , _uniformAllocator(uniformAllocator)
            , _device(device)
        {
        }

        ugi::Material* createMaterial(std::vector<std::string>const & params);

        ugi::Renderable* createRenderable(uint8_t const* vd, uint32_t vdsize, uint16_t const* id, uint32_t indexCount, GPUAsyncLoadManager* asyncLoadManager);

        void setUBO(Renderable* renderable, uint8_t* data);
        void setSampler(Renderable* renderable, sampler_state_t sampler);
        void setTexture(Renderable* renderable, image_view_t imageView);
        void setRasterization(raster_state_t rasterState);
        void bind(RenderCommandEncoder* encoder);
        void draw(RenderCommandEncoder* enc, Renderable* renderable);

        bool initialize();

        void tick();
    };

    class HelloWorld : public UGIApplication {
    private:
        void*                           _hwnd;                                             //
        StandardRenderContext*          _renderContext; //
        Render*                         _render;
        // sample resources
        ugi::Texture*                   _textures[2];
        ugi::image_view_t               _imageViews[2];
        ugi::sampler_state_t            _samplerState;                                     //
        // a renderable object
        ugi::Renderable*                _renderable; 
        //
        uint32_t                        _flightIndex;                                      // flight index
        //
        float                           _width;
        float                           _height;
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