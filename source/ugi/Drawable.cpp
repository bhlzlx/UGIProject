#include "Drawable.h"
#include "CommandBuffer.h"
#include "Buffer.h"
#include "VulkanFunctionDeclare.h"
#include <cassert>

namespace ugi {

    uint32_t Drawable::vertexBufferCount() const {
        return _vertexBufferCount;
    }

    void Drawable::setVertexBuffer( Buffer* buffer, uint32_t index, uint64_t offset ) {
        assert( index < _vertexBufferCount ) ;
        if( index < _vertexBufferCount ) {
            _vertexBuffers[index] = buffer;
            _vertexBufferOffsets[index] = offset;
        }
    }

    void Drawable::setIndexBuffer( Buffer* buffer, uint64_t offset ) {
        _indexBuffer = buffer;
        _indexBufferOffset = offset;
    }

    void Drawable::bind( const CommandBuffer* commandBuffer ) const {
        VkBuffer vbuf[MaxVertexBufferBinding];
        for( uint32_t i = 0; i<vertexBufferCount(); ++i) {
            vbuf[i] = _vertexBuffers[i]->buffer();
        }
        vkCmdBindVertexBuffers( *commandBuffer, 0,vertexBufferCount(), vbuf, _vertexBufferOffsets );
        vkCmdBindIndexBuffer( *commandBuffer, _indexBuffer->buffer(), _indexBufferOffset, VkIndexType::VK_INDEX_TYPE_UINT16 );
    }

}