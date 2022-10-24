#pragma once
#include <LightWeightCommon/memory/tlsf/comm_tlsf.h>

namespace ugi {

    enum class vtxbuff_type {
        static_mesh = 0,
        charactor,
        terrain,
        particles,
    };

    struct vtxbuff_t {
        vtxbuff_type        type;
        uint32_t            index;
        uint32_t            offset; 
        uint32_t            size;
    };

    using vtxbuff_handle_t = vtxbuff_t*;

    class GPUBufferAllocator {
    private:
        comm::tlsf::Pool        tpool_; // tlsf pool

    public:
    };

}