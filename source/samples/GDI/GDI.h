#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <ugi/Argument.h>
#include <ugi/ResourceManager.h>
#include <UI/UI2D.h>

namespace ugi {

    class GDISample : public UGIApplication {
    private:
        void*                   m_hwnd;                                             //
        ugi::RenderSystem*      m_renderSystem;                                     //
        ugi::Device*            m_device;                                           //
        ugi::Swapchain*         m_swapchain;                                        //
        //
        ugi::Fence*             m_frameCompleteFences[MaxFlightCount];              // command buffer 被GPU消化完会给 fence 一个 signal, 用于双缓冲或者多缓冲逻辑隔帧等待
        ugi::Semaphore*         m_renderCompleteSemaphores[MaxFlightCount];         // 用于GPU内部等待
        ugi::CommandBuffer*     m_commandBuffers[MaxFlightCount];                   // command buffer for each frame
        ugi::CommandQueue*      m_graphicsQueue;                                    // graphics queue
        ugi::CommandQueue*      m_uploadQueue;                                      // upload queue
        ugi::Pipeline*          m_pipeline;
        ugi::ResourceManager*   m_resourceManager;                                  // resource manager
        ///> ===========================================================================
        ugi::ArgumentGroup*     m_argumentGroup;                                    //
        
        ugi::gdi::GDIContext*   m_gdiContext;
        ugi::gdi::GeometryGPUDrawData* m_geomDrawData;                              //
        ugi::UniformAllocator*  m_uniformAllocator;
        //
        uint32_t                m_flightIndex;                                      // flight index
        //
        float                   m_width;
        float                   m_height;
    public:
        virtual bool initialize( void* _wnd,  hgl::assets::AssetsSource* assetsSource );
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
    };

}

UGIApplication* GetApplication();