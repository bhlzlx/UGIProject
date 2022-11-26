#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <vector>
// #include <ugi/descriptor_binder.h>

namespace ugi {

    struct GaussBlurParameter {
        float       direction[2];
        uint32_t    radius;
        uint32_t    padding;
        float       gaussDistribution[12];
    };

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

    class GaussBlurTest : public UGIApplication {
    private:
        void*                           hwnd_;                                             //
        StandardRenderContext*          renderContext_;

        ComputePipeline*                pipeline_;
        Material*                       pass1mtl_;
        Material*                       pass2mtl_;

        ugi::Texture*                   texture_;
        ugi::Texture*                   blurTextures_[2];
        ugi::image_view_t               blurImageViews_[2];
        ugi::Material*                  blurMaterials_[2];
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