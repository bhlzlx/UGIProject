#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include <ugi/flight_cycle_invoker.h>
#include <gdi/gdi.h>
#include <gdi/geometry/geometryBuilder.h>
#include <gdi/geometry/geometryDrawData.h>

namespace ugi {

    class GDISample : public UGIApplication {
    private:
        void*                           _hwnd;                                             //
        ugi::RenderSystem*              _renderSystem;                                     //
        ugi::Device*                    _device;                                           //
        ugi::Swapchain*                 _swapchain;                                        //
        //      
        ugi::Fence*                     _frameCompleteFences[MaxFlightCount];              // command buffer 被GPU消化完会给 fence 一个 signal, 用于双缓冲或者多缓冲逻辑隔帧等待
        ugi::Semaphore*                 _renderCompleteSemaphores[MaxFlightCount];         // 用于GPU内部等待
        ugi::CommandBuffer*             _commandBuffers[MaxFlightCount];                   // command buffer for each frame
        ugi::CommandQueue*              _graphicsQueue;                                    // graphics queue
        ugi::CommandQueue*              _uploadQueue;                                      // upload queue
        ugi::GraphicsPipeline*          _pipeline;
        ugi::ResourceManager*           _resourceManager;                                  // resource manager
        ///> ===========================================================================
        ugi::ArgumentGroup*             _argumentGroup;                                    //
        
        ugi::gdi::GDIContext*           _gdiContext;
        ugi::gdi::IGeometryBuilder*     _geomBuilder;
        ugi::gdi::GeometryDrawData*     _geomDrawData;
        ugi::UniformAllocator*          _uniformAllocator;
        //
        uint32_t                        _flightIndex;                                      // flight index
        //
        float                           _width;
        float                           _height;
    public:
        virtual bool initialize( void* wnd,  hgl::assets::AssetsSource* assetsSource );
        virtual void resize( uint32_t width, uint32_t height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
    };

}

UGIApplication* GetApplication();