#pragma once
#include "commandBuffer/ResourceCommandEncoder.h"
#include "commandBuffer/RenderCommandEncoder.h"
#include "commandBuffer/ComputeCommandEncoder.h"
#include "UGITypes.h"
#include <cstdint>

namespace ugi {
    
    class CommandBuffer;

    class ResourceCommandEncoder;
    class RenderCommandEncoder;

    class CommandBuffer {
		friend class CommandQueue;
    private:
        VkCommandBuffer                                 _cmdbuff;
        VkDevice                                        _device;
        union {
            uint32_t                                    _encodeState;
            ResourceCommandEncoder                      _resourceEncoder;
            RenderCommandEncoder                        _renderEncoder;
            ComputeCommandEncoder                       _computeEncoder;
        };
    private:
		~CommandBuffer() = default;
    public:
        CommandBuffer( VkDevice device, VkCommandBuffer cb );
        //
        operator VkCommandBuffer() const;
        VkDevice device() const;

        ResourceCommandEncoder* resourceCommandEncoder();
        RenderCommandEncoder* renderCommandEncoder( IRenderPass* renderPass );
        ComputeCommandEncoder* computeCommandEncoder();
        //
        void reset();
        void beginEncode();
        void endEncode();
    };

}