#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <ugi/Argument.h>

#include <io/archive.h>

namespace ugi {

    struct Material {
        
    };

    struct RenderContext {
        ugi::RenderSystem*              renderSystem;                                     //
        ugi::Device*                    device;                                           //
        ugi::Swapchain*                 swapchain;                                        //
        ugi::Fence*                     frameCompleteFences[MaxFlightCount];              // command buffer 被GPU消化完会给 fence 一个 signal, 用于双缓冲或者多缓冲逻辑隔帧等待
        ugi::Semaphore*                 renderCompleteSemaphores[MaxFlightCount];         // 用于GPU内部等待
        ugi::CommandBuffer*             commandBuffers[MaxFlightCount];                   // command buffer for each frame
        ugi::CommandQueue*              graphicsQueue;                                    // graphics queue
        ugi::CommandQueue*              uploadQueue;                                      // upload queue
        ugi::UniformAllocator*          uniformAllocator;
        ugi::DescriptorSetAllocator*    descriptorSetAllocator;
        ugi::GPUAsyncLoadManager*       asyncLoadManager;

        RenderContext() {
        }

        bool initialize(void* _wnd, ugi::DeviceDescriptor deviceDesc, common::IArchive* archive);
    };

    class Render {
    private:
        ugi::GraphicsPipeline*          _pipeline;
    public:
        Render(ugi::GraphicsPipeline* pipeline)
            : _pipeline(pipeline)
        {
        }
    };

    class HelloWorld : public UGIApplication {
    private:
        void*                   _hwnd;                                             //
        RenderContext           _renderContext; //
        ugi::GraphicsPipeline*  _pipeline;
        ///> ===========================================================================
        // ugi::Buffer*            m_uniformBuffer;
        ugi::Texture*           _texture;
        ugi::image_view_t       _imageView; 
        ugi::sampler_state_t    _samplerState;                                     //
        ugi::Buffer*            _vertexBuffer;                                     //
        ugi::Buffer*            _indexBuffer;
        ugi::Drawable*          _drawable;

        res_descriptor_t        _uniformDescriptor;
        //
        uint32_t                _flightIndex;                                      // flight index
        //
        float                   _width;
        float                   _height;
    public:
        virtual bool initialize(void* _wnd, common::IArchive* arch);
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
    };

}

UGIApplication* GetApplication();