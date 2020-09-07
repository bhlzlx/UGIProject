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
        float       gaussDistribution[8];
    };

    class GaussBlurItem {
    private:
        ArgumentGroup*      _argGroup;
        Texture*            _texture0;
        Texture*            _texture1;
        GaussBlurParameter  _parameter;
    public:
        GaussBlurItem( ArgumentGroup* group, Texture* texture0, Texture* texture1 );
        void setParameter( const GaussBlurParameter& parameter );
        const GaussBlurParameter& parameter() const {
            return _parameter;
        }
        ArgumentGroup* argumentGroup() const {
            return _argGroup;
        }
        //
        Texture* target() const;
    };

    class GaussBlurProcessor {
    private:
        ugi::Device*                    _device;
        hgl::assets::AssetsSource*      _assetsSource;
        ugi::ComputePipeline*           _pipeline;
        PipelineDescription             _pipelineDescription;
        //
        uint32_t                        _inputImageHandle;
        uint32_t                        _outputImageHandle;
        uint32_t                        _parameterHandle;
    public:
        GaussBlurProcessor();
        //
        bool intialize( ugi::Device* device, hgl::assets::AssetsSource* assetsSource );
        //
        void prepareResource( GaussBlurItem* task, ResourceCommandEncoder* encoder, UniformAllocator* uniformAllocator );
        void processBlur( GaussBlurItem* task, ComputeCommandEncoder* computeEncoder );

        GaussBlurItem* createGaussBlurItem( ugi::Texture* texture0, ugi::Texture* texture1 );
    };

}