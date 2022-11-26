#pragma once
#include "../ugi_declare.h"
#include "../vulkan_declare.h"
#include "../ugi_types.h"

namespace ugi {


    class ComputeCommandEncoder {
    private:
        CommandBuffer*  _commandBuffer;
    public:
        ComputeCommandEncoder( CommandBuffer* commandBuffer = nullptr )
            : _commandBuffer( commandBuffer )
        {
        }

        void bindPipeline(ComputePipeline* computePipeline);
        void bindDescriptors(DescriptorBinder* binder);
        
        void dispatch( uint32_t groupX, uint32_t groupY, uint32_t groupZ ) const;
        //
        CommandBuffer* commandBuffer() const;

        void endEncode() {
            _commandBuffer = nullptr;
        }
    };

}