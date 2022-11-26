#include "compute_cmd_encoder.h"
#include "../vulkan_function_declare.h"
#include "../command_buffer.h"
#include "../pipeline.h"
#include "../descriptor_binder.h"

namespace ugi {

    void ComputeCommandEncoder::dispatch( uint32_t groupX, uint32_t groupY, uint32_t groupZ ) const {
        VkCommandBuffer cmdbuf = * _commandBuffer;
        vkCmdDispatch(cmdbuf, groupX, groupY, groupZ);
    }

    void ComputeCommandEncoder::bindPipeline( ComputePipeline* computePipeline ) {
        VkCommandBuffer cmdbuf = * _commandBuffer;
        vkCmdBindPipeline( cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->pipeline() );
    }

    void ComputeCommandEncoder::bindDescriptors(DescriptorBinder* binder) {
        binder->bind(_commandBuffer);
    }

    CommandBuffer* ComputeCommandEncoder::commandBuffer() const {
        return _commandBuffer;
    }

}
