#pragma once

#include <ugi_types.h>
#include <vector>
#include <mesh_buffer_allocator.h>
#include <functional>

namespace ugi {

    class Mesh {
        friend class GPUMeshAsyncLoadItem;
    private:
        MeshBufferAllocator*    meshbufferAllocator;
        mesh_buffer_handle_t    buffer_;
        // uint32_t                vboffset_;
        uint32_t                iboffset_;
        size_t                  indexCount_; // index count of primitive
        uint32_t                attriCount_;
        uint64_t                attriOffsets_[MaxVertexAttribute];
        //
        vertex_layout_t         vertexLayout_;
        topology_mode_t         topologyMode_;
        polygon_mode_t          polygonMode_;
        uint8_t                 uploaded_:1;
    public:
        Mesh()
            : meshbufferAllocator(nullptr)
            , buffer_(mesh_buffer_handle_invalid)
            // , vboffset_(0)
            , iboffset_(0)
            , indexCount_(0)
            , attriCount_(0)
            , attriOffsets_{}
            , topologyMode_(topology_mode_t::TriangleList)
            , polygonMode_(polygon_mode_t::Line)
            , uploaded_(0)
        {}

        mesh_buffer_alloc_t buffer() const {
            return meshbufferAllocator->deref(buffer_);
        }

        uint32_t attributeCount() const {
            return attriCount_;
        }

        uint32_t indexCount() const {
            return indexCount_;
        }

        topology_mode_t topologyMode() const {
            return topologyMode_;
        }

        polygon_mode_t polygonMode() const {
            return polygonMode_;
        }

        uint32_t iboffset() const {
            return iboffset_;
        }

        bool prepared() const {
            return uploaded_;
        }

        void bind(const RenderCommandEncoder* encoder) const;

        static Mesh* CreateMesh(
            Device* device,
            MeshBufferAllocator* allocator,
            GPUAsyncLoadManager* asyncLoadManager,
            uint8_t const* vb, uint32_t vbSize, 
            uint16_t const* indice, uint32_t indexCount,
            vertex_layout_t layout,
            topology_mode_t topologyMode,
            polygon_mode_t polygonMode,
            std::function<void(void*,CommandBuffer*)> onComplete
        );

    };
    

}