#pragma once

#include "UGITypes.h"
#include "UGIDeclare.h"
#include "VulkanDeclare.h"
#include <cstdint>

namespace ugi {

    class Drawable {
    private:
        uint32_t            _vertexBufferCount;
        Buffer*             _vertexBuffers;
        VkDeviceSize        _vertexBufferOffsets[MaxVertexBufferBinding];
        Buffer*             _indexBuffer;
        VkDeviceSize        _indexBufferOffset;
    public:
        Drawable(uint32_t vertexBufferCount)
            : _vertexBufferCount(vertexBufferCount)
            , _vertexBuffers {}
            , _vertexBufferOffsets {}
            , _indexBuffer(nullptr)
            , _indexBufferOffset(0)
        {
        }
        uint32_t vertexBufferCount() const;
        void setVertexBuffer(Buffer* buffer, uint32_t index, uint64_t offset );
        void setIndexBuffer( Buffer* buffer, uint64_t offset );
        void bind( const CommandBuffer* commandBuffer ) const;

    };

}