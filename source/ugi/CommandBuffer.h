#pragma once
#include "commandBuffer/ResourceCommandEncoder.h"
#include "commandBuffer/RenderCommandEncoder.h"
#include "UGITypes.h"
#include <cstdint>

namespace ugi {

    struct ImageRegion {
        struct Offset {
            int32_t x; int32_t y; int32_t z;
            Offset( int32_t x = 0, int32_t y = 0, int32_t z = 0)
                : x(x)
                , y(y)
                , z(z)
            {
            }
        };
        struct Extent {
            uint32_t width; uint32_t height; uint32_t depth;
            Extent( uint32_t width = 1, uint32_t height = 1, uint32_t depth = 1)
                : width(width)
                , height(height)
                , depth(depth)
            {
            }
        };
        uint32_t    mipLevel;
        uint32_t    arrayIndex;        // texture array index only avail for texture array
        Offset      offset;
        Extent      extent;
        //
        ImageRegion( uint32_t mipLevel = 0, uint32_t arrayIndex = 0 )
            : mipLevel(mipLevel)
            , arrayIndex(arrayIndex)
            , offset()
            , extent()
        {
        }
    };

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
        //
        void reset();
        void beginEncode();
        void endEncode();
    };

}