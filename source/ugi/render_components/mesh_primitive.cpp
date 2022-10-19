#include "mesh_primitive.h"
#include <VulkanDeclare.h>
#include <VulkanFunctionDeclare.h>
#include <CommandBuffer.h>
#include <Buffer.h>
#include <Device.h>

namespace ugi {

    static_assert(sizeof(uint64_t) == sizeof(VkDeviceSize),"must be same");

    void Mesh::bind(const CommandBuffer* cb) const {
        VkBuffer vbuf[MaxVertexBufferBinding];
        for( uint32_t i = 0; i<this->attriCount_; ++i) {
            vbuf[i] = this->vertices_->buffer();
        }
        vkCmdBindVertexBuffers(*cb, 0, attriCount_, vbuf, (VkDeviceSize*)attriOffsets_);
        vkCmdBindIndexBuffer( *cb, indices_->buffer(), indexOffset_, VkIndexType::VK_INDEX_TYPE_UINT16);
    }

    /**
     * @brief 
     *    由于创建资源，需要等待一个队列传输数据，所以，需要额外创建一个队列，以及额外创建一个线程，需要程序有一个异步创建的机制。
     * @param vb 
     * @param vbSize 
     * @param indice 
     * @param indexCount 
     * @param layout 
     * @param topologyMode 
     * @param polygonMode 
     * @return Mesh* 
     */
    Mesh* Mesh::CreateMesh(
        Device* device, // device
        uint8_t const* vb, uint64_t vbSize, 
        uint16_t const* indice, uint64_t indexCount,
        vertex_layout_t layout,
        topology_mode_t topologyMode,
        polygon_mode_t polygonMode
    ) {
        Mesh* mesh = new Mesh();
        for(auto i = 0; i<layout.bufferCount; ++i) {
            mesh->attriOffsets_[i] = layout.buffers[i].offset;
        }
    }

}