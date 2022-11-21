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
        void*                           hwnd_;                                             //
        StandardRenderContext*          renderContext_;
        ugi::Texture*                   texture_;
        ugi::Texture*                   bluredTexture_;
        ugi::Texture*                   bluredTextureFinal_;
        ugi::GaussBlurProcessor*        gaussProcessor_;
        ugi::GaussBlurItem*             blurItem_;
        ugi::GaussBlurItem*             blurItem2_;
        float                           width_;
        float                           height_;
    public:
        virtual bool initialize(void* _wnd, comm::IArchive* archive);
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
    };

}

UGIApplication* GetApplication();