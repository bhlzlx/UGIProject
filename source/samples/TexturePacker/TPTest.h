﻿#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include "SDFFontRenderer.h"

namespace ugi {

    class TPTest : public UGIApplication {
    private:
        void*                   _hwnd;                                             //
        ugi::RenderSystem*      _renderSystem;                                     //
        ugi::Device*            _device;                                           //
        ugi::Swapchain*         _swapchain;                                        //
        //
        ugi::Fence*             _frameCompleteFences[MaxFlightCount];              // command buffer 被GPU消化完会给 fence 一个 signal, 用于双缓冲或者多缓冲逻辑隔帧等待
        ugi::Semaphore*         _renderCompleteSemaphores[MaxFlightCount];         // 用于GPU内部等待
        ugi::CommandBuffer*     _commandBuffers[MaxFlightCount];                   // command buffer for each frame
        ugi::CommandQueue*      _graphicsQueue;                                    // graphics queue
        ugi::CommandQueue*      _uploadQueue;                                      // upload queue
        ugi::GraphicsPipeline*          _pipeline;
        
        ugi::ResourceManager*   _resourceManager;
        ugi::UniformAllocator*  _uniformAllocator;
        ///> ===========================================================================
        ugi::SDFFontRenderer*   _fontRenderer;
        ugi::SDFFontDrawData*   _drawData;

        IndexHandle             _h1, _h2, _h3, _h4;
        
        uint32_t                _flightIndex;                                      // flight index
        //
        float                   _width;
        float                   _height;
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