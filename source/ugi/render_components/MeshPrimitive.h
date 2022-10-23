#pragma once

#include <UGITypes.h>
#include <vector>

namespace ugi {

    class Mesh {
        friend class GPUMeshAsyncLoadItem;
    private:
        Buffer*         vertices_;
        Buffer*         indices_;
        uint32_t        attriCount_;
        uint64_t        attriOffsets_[MaxVertexAttribute];
        size_t          indexOffset_;
        size_t          indexCount_; // index count of primitive
        //
        vertex_layout_t vertexLayout_;
        topology_mode_t topologyMode_;
        polygon_mode_t  polygonMode_;
        uint8_t         prepared_:1;
    public:
        Mesh()
            : vertices_(nullptr)
            , indices_(nullptr)
            , attriCount_(0)
            , attriOffsets_{}
            , indexOffset_(0)
            , indexCount_(0)
            , topologyMode_(topology_mode_t::TriangleList)
            , polygonMode_(polygon_mode_t::Line)
            , prepared_(0)
        {}

        Buffer* vertexBuffer() const {
            return vertices_;
        }

        Buffer* indexBuffer() const {
            return indices_;
        }

        uint32_t attributeCount() const {
            return attriCount_;
        }

        topology_mode_t topologyMode() const {
            return topologyMode_;
        }

        polygon_mode_t polygonMode() const {
            return polygonMode_;
        }

        bool prepared() const {
            return prepared_;
        }

        void bind(const RenderCommandEncoder* encoder) const;

        static Mesh* CreateMesh(
            Device* device,
            GPUAsyncLoadManager* asyncLoadManager,
            uint8_t const* vb, uint64_t vbSize, 
            uint16_t const* indice, uint64_t indexCount,
            vertex_layout_t layout,
            topology_mode_t topologyMode,
            polygon_mode_t polygonMode
        );

    };
    

}