#pragma once
#include "../UGIDeclare.h"
#include "../VulkanDeclare.h"
#include "../UGITypes.h"

namespace ugi {


    class ComputeCommandEncoder {
    private:
        CommandBuffer*  _commandBuffer;
    public:
        ComputeCommandEncoder( CommandBuffer* commandBuffer = nullptr )
            : _commandBuffer( commandBuffer )
        {
        }

        void bindPipeline( ComputePipeline* computePipeline );
        void bindArgumentGroup( DescriptorBinder* argumentGroup );
        
        void dispatch( uint32_t groupX, uint32_t groupY, uint32_t groupZ ) const;
        //
        CommandBuffer* commandBuffer() const;

        void endEncode() {
            _commandBuffer = nullptr;
        }
    };

}