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
        void*                   hwnd_;                                             //
        StandardRenderContext   renderContext_;
        ugi::Texture*           _texture;
        ugi::Texture*           _bluredTexture;
        ugi::Texture*           _bluredTextureFinal;
        //
        ugi::GaussBlurProcessor*    _gaussProcessor;
        ugi::GaussBlurItem*         _blurItem;
        ugi::GaussBlurItem*         _blurItem2;

        ugi::UniformAllocator*  _uniformAllocator;
        res_descriptor_t      _uniformDescriptor;
        //
        uint32_t                _flightIndex;                                      // flight index
        //
        float                   _width;
        float                   _height;
    public:
        virtual bool initialize( void* _wnd, comm::IArchive* archive);
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
    };

}

UGIApplication* GetApplication();