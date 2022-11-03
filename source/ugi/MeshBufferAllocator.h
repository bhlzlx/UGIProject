#pragma once
#include <VulkanDeclare.h>
#include <UGITypeMapping.h>
#include <UGITypes.h>
#include <UGIDeclare.h>
#include <vk_mem_alloc.h>
#include <LightWeightCommon/memory/tlsf/comm_tlsf.h>
#include <atomic>
#include <mutex>

namespace ugi {

    /**
     * @brief 
     *     整理buffer这个工作,一定要无缝切换,所以我们一定得保证下一帧数据一定是有效的,得利用Fence去做这件事.
     * 所以我们得设计好这个数据结构和逻辑。
     * 我们得考虑，更新mesh如何通知这个mesh已经更新完毕，或者要有一个机制能保证它一定更新完毕。
     * 更新数据，我们可以分为两类
     * 1. 初始化mesh
     * 2. 合批这种更新mesh
     * 这两种有什么不同呢？初始化mesh不需要下一帧立即完成，而重组织buffer这种需要下一帧立即可用。
     * 所以，前者我们需要在渲染线程每帧去判断是否加载完毕就行，后者，我们需要下一帧有一个等待机制。
     * 所以这两种我们都需要使用到fence，只是每帧tick的时候处理的方式不一样。
     */

    struct mesh_buffer_alloc_t {
        VkBuffer buffer;
        uint32_t offset;
        uint32_t length;
        uint32_t blockIndex:16;
        uint32_t uploaded:1;
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

    // 查找过程如下
    // handle -> alloc -> block

    // allocator 可以以主线程里用，也可以在子线程里用，都不影响GPU数据上传的状态在渲染线程同步
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
        std::atomic<uint8_t>                            rearrangeCounter_;
        std::mutex                                      mutex_; // 这个allocator应用场景应该也只限于单线程，所以这里没啥用
        // rearrange old buffer blocks
        std::vector<buffer_block_t>                     rearrangingBlocks_;
        std::vector<mesh_buffer_alloc_t>                rearrangingAllocs_;
        std::vector<mesh_buffer_handle_t>               destroyRecords_;
    private:
        bool createNewBufferBlock_(uint32_t size);
        uint32_t allocID_();
        mesh_buffer_alloc_t deref_(mesh_buffer_handle_t handle) const;
    public:
        MeshBufferAllocator()
            : bufferBlocks_{}
            , bufferAllocs_{}
            , freeIDs_{}
            , allocationCount_(0)
            , device_(nullptr)
            , rearrangeCounter_(false)
        {}
        bool initialize(Device* device, uint32_t poolSize);
        void rearrangeBufferAlloc(ResourceCommandEncoder* encoder); // 应该在每帧结束，要提交之前执行
        mesh_buffer_handle_t alloc(uint32_t size);
        bool free(mesh_buffer_handle_t buf);
        void onFrameTick(); // 应该在每帧开始时调用
    };



}