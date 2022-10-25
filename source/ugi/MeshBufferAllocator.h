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
    };

    struct mesh_buffer_handle_t {
        union {
            uint32_t id; 
            struct {
                uint16_t block;
                uint16_t index;
            };
        };
        static constexpr uint32_t INVALID_HANDLE = 0xffffffff;
    };

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
        std::vector<std::vector<mesh_buffer_alloc_t>>   bufferAllocs_;     // buffer allocations
        std::vector<std::vector<uint16_t>>              freeIndices_;      // free allocation locations
        size_t                                          allocationCount_;  // 
        Device*                                         device_;
        uint32_t                                        poolSize_;
        uint32_t                                        totalSize_;
    private:
        bool createNewBufferBlock(uint32_t size);
        uint16_t allocLoc(size_t blockIndex);
        mesh_buffer_alloc_t deref(mesh_buffer_handle_t handle) const;
    public:
        MeshBufferAllocator()
            : bufferBlocks_{}
            , bufferAllocs_{}
            , freeIndices_{}
            , allocationCount_(0)
            , device_(nullptr)
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
        void reorganizeAllocations(ResourceCommandEncoder* encoder, std::vector<mesh_buffer_alloc_t*> const& allocations);
        void onFrameTick();
    };

}