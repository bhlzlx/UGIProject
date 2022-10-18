#pragma once

#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {

    struct GaussBlurParameter {
        float       direction[2];
        uint32_t    radius;
        uint32_t    padding;
        float       gaussDistribution[12];
    };

    class GaussBlurItem {
    private:
        DescriptorBinder*      _argGroup;
        Texture*            _texture0;
        Texture*            _texture1;
        GaussBlurParameter  _parameter;
    public:
        GaussBlurItem( DescriptorBinder* group, Texture* texture0, Texture* texture1 );
        void setParameter( const GaussBlurParameter& parameter );
        const GaussBlurParameter& parameter() const {
            return _parameter;
        }
        DescriptorBinder* argumentGroup() const {
            return _argGroup;
        }
        Texture* source() const {
            return _texture0;
        }
        //
        Texture* target() const {
            return _texture1;   
        }
    };

    class GaussBlurProcessor {
    private:
        ugi::Device*                    _device;
        hgl::assets::AssetsSource*      _assetsSource;
        ugi::ComputePipeline*           _pipeline;
        pipeline_desc_t             _pipelineDescription;
        //
        uint32_t                        _inputImageHandle;
        uint32_t                        _outputImageHandle;
        uint32_t                        _parameterHandle;
    public:
        GaussBlurProcessor();
        //
        bool intialize( ugi::Device* device, hgl::assets::AssetsSource* assetsSource );
        //
        void processBlur( GaussBlurItem* item, CommandBuffer* commandBuffer, UniformAllocator* uniformAllocator );

        GaussBlurItem* createGaussBlurItem( ugi::Texture* texture0, ugi::Texture* texture1 );
    };

}