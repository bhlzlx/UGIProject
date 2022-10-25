#pragma once
#include <VulkanDeclare.h>
#include <UGITypeMapping.h>
#include <UGITypes.h>
#include <UGIDeclare.h>
#include <vk_mem_alloc.h>
#include <LightWeightCommon/memory/tlsf/comm_tlsf.h>

namespace ugi {

    struct mesh_buffer_alloc_t {
        VkBuffer buffer;
        uint32_t offset;
        uint32_t length;
        uint32_t blockIndex;
    };

    using mesh_buffer_handle_t = uint32_t;
    constexpr uint32_t mesh_buffer_handle_invalid = 0xffffffff;

    struct buffer_block_t {
        VkBuffer                buffer;         // vk buffer
        VmaAllocation           allocation;     // vma allocation
        size_t                  bufferSize;     // size of buffer block
        comm::tlsf::Pool        scheduler;      // tlsf scheduler
        //
        buffer_block_t(VkBuffer buf, VmaAllocation alloc, size_t size)
            : buffer(buf)
            , allocation(alloc)
            , bufferSize(size)
            , scheduler(size)
        {}
        buffer_block_t(buffer_block_t && block)
            : buffer(block.buffer)
            , allocation(block.allocation)
            , bufferSize(block.bufferSize)
            , scheduler(std::move(block.scheduler))
        {}
    };

    class MeshBufferAllocator {
    private:
        std::vector<buffer_block_t>                     bufferBlocks_;     // buffer pools 
        std::vector<mesh_buffer_alloc_t>                bufferAllocs_;
        std::vector<uint32_t>                           freeIDs_;
        size_t                                          allocationCount_;  // 
        Device*                                         device_;
        uint32_t                                        poolSize_;
        uint32_t                                        totalSize_;
        //
        bool                                            rearrangeState_;
    private:
        bool createNewBufferBlock(uint32_t size);
        uint32_t allocID();
        mesh_buffer_alloc_t deref(mesh_buffer_handle_t handle) const;
    public:
        MeshBufferAllocator()
            : bufferBlocks_{}
            , bufferAllocs_{}
            , freeIDs_{}
            , allocationCount_(0)
            , device_(nullptr)
            , rearrangeState_(false)
        {}
        bool initialize(Device* device, uint32_t poolSize);
        mesh_buffer_handle_t alloc(uint32_t size);
        bool free(mesh_buffer_handle_t buf);
        /**
         * @brief 
         * 
         * @param encoder 录制指令
         * @param allocations 必须得保证所有的分配记录都在里面
         */
        void rearrangeBufferAlloc(ResourceCommandEncoder* encoder, std::vector<mesh_buffer_alloc_t*> const& allocations);
        void onFrameTick();
    };

}