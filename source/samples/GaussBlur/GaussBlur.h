#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <ugi/Argument.h>

namespace ugi {

    std::vector<float> GenerateGaussDistribution( float sigma ) {
        sigma = sigma > 0 ? sigma : -sigma;  
        int ksize = ceil(sigma * 3) * 2 + 1;
        std::vector<float> distribution(ksize);
        //计算一维高斯核
        float scale = -0.5f/(sigma*sigma);  
        const double PI = 3.141592653f;  
        float cons = 1/sqrt(-scale / PI);  
        float sum = 0.0f;  
        int kcenter = ksize/2;  
        int i = 0;
        for(i = 0; i < ksize; i++) {  
            int x = i - kcenter;  
            distribution[i] = cons * exp(x * x * scale);
            sum += distribution[i] ;
        }
        for(i = 0; i < ksize; i++) {  
            distribution[i] /= sum;  
        }
        return distribution;
    }

    class GaussBlurProcessor;
    class GaussBlurItem;
    

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
        ugi::GraphicsPipeline*          _pipeline;
        ///> ===========================================================================
        ugi::ArgumentGroup*     _argumentGroup;                                    // 
        // ugi::Buffer*            m_uniformBuffer;
        ugi::Texture*           _texture;
        ugi::Texture*           _bluredTexture;
        ugi::Texture*           _bluredTextureFinal;
        //
        ugi::GaussBlurProcessor*    _gaussProcessor;
        ugi::GaussBlurItem*         _blurItem;
        ugi::GaussBlurItem*         _blurItem2;

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