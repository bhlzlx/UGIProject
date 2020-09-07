#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <ugi/Argument.h>

namespace ugi {

    class GaussBlurProcessor;
    class GaussBlurItem;

    class TextureUtility {
    private:
        Device*             _device;
        CommandQueue*       _transferQueue;
        CommandQueue*       _graphicsQueue;
    public:
        TextureUtility( Device* device, CommandQueue* transferQueue, CommandQueue* graphicsQueue )
            : _device(device)
            , _transferQueue(transferQueue)
            , _graphicsQueue(graphicsQueue) {
        }
        // replace texture
        void replaceTexture( CommandQueue* transferQueue, Texture* texture, const ImageRegion* regions,void* data, uint32_t dataLength, uint32_t* offsets, uint32_t regionCount );
    };

    class GaussBlurTest : public UGIApplication {
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
        ugi::Pipeline*          _pipeline;
        ///> ===========================================================================
        ugi::ArgumentGroup*     _argumentGroup;                                    // 
        // ugi::Buffer*            m_uniformBuffer;
        ugi::Texture*           _texture;
        ugi::Texture*           _bluredTexture;
        //
        ugi::GaussBlurProcessor*    _gaussProcessor;
        ugi::GaussBlurItem*         _blurItem;

        ugi::UniformAllocator*  _uniformAllocator;
        ResourceDescriptor      _uniformDescriptor;
        //
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