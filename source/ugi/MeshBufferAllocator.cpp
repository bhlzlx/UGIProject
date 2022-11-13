#include "MeshBufferAllocator.h"
#include <Device.h>
#include <CommandBuffer.h>
#include <cassert>

namespace ugi {

    VkBufferUsageFlags getVkBufferUsageFlags(BufferType _type);
    VmaMemoryUsage getVmaAllocationUsage(BufferType _type);

    bool MeshBufferAllocator::createNewBufferBlock_(uint32_t size) {
        if(size<poolSize_) {
            size = poolSize_;
        }
        size = (size+127)&(~127);
        VkBufferCreateInfo bufferInfo = {}; {
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.flags = 0;
            bufferInfo.pNext = nullptr;
            bufferInfo.pQueueFamilyIndices = device_->descriptor().queueFamilyIndices;
            bufferInfo.queueFamilyIndexCount = device_->descriptor().queueFamilyCount;
            if(device_->descriptor().queueFamilyCount>1) {
                bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            } else {
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            bufferInfo.size = size;
            bufferInfo.usage = getVkBufferUsageFlags(BufferType::VertexBuffer) | getVkBufferUsageFlags(BufferType::IndexBuffer);
        }
        VmaAllocationCreateInfo allocationCreateInfo = {}; {
            allocationCreateInfo.usage = getVmaAllocationUsage(BufferType::VertexBuffer);
        }
        //
        auto vmaAllocator = device_->vmaAllocator();
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkResult rst = vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocationCreateInfo, &buffer, &allocation, nullptr );
        if(rst == VK_SUCCESS){
            bufferBlocks_.emplace_back(buffer, allocation, size);
            totalSize_ += size;
            return true;
        }
        assert(false);
        return false;
    }

    bool MeshBufferAllocator::initialize(Device* device, uint32_t poolSize) {
        if(device_) {
            return false;
        }
        poolSize_ = poolSize;
        device_ = device;
        rearrangeCounter_ = 0;
        auto rst = createNewBufferBlock_(poolSize);
        return rst;
    }

    std::pair<mesh_buffer_handle_t, mesh_buffer_alloc_t> MeshBufferAllocator::alloc(uint32_t size) {
        // TODO: 添加处理重排时的逻辑
        std::unique_lock<std::mutex> lock(mutex_);
        uint16_t blockIndex = 0;
        if(rearrangeCounter_) {
            blockIndex = 1; // 跳过第一个block，因这重排之后，第一个block就是重排之后的，它现在还不能立即使用。
        }
        mesh_buffer_alloc_t alloc = {};
        for(blockIndex = 0; blockIndex<bufferBlocks_.size(); ++blockIndex) {
            auto& block = bufferBlocks_[blockIndex];
            auto offset = block.scheduler.alloc(size);
            if(offset == ~0) {
                // alloc failed, try next pool
                continue;
            } else {
                alloc.buffer = bufferBlocks_[blockIndex].buffer;
                alloc.length = size;
                alloc.offset = offset;
                alloc.blockIndex = blockIndex;
                break;
            }
        }
        if(!alloc.buffer) { // alloc failed, need a new pool
            bool rst = createNewBufferBlock_(size);
            if(!rst) {
                return { mesh_buffer_handle_invalid, {} };
            }
            auto offset = bufferBlocks_.back().scheduler.alloc(size);
            alloc =  {bufferBlocks_.back().buffer, offset, size};
            blockIndex = bufferBlocks_.size() - 1;
        }
        mesh_buffer_handle_t id = allocID_();
        bufferAllocs_[id] = alloc;
        return { id, alloc };
    }

    uint32_t MeshBufferAllocator::allocID_() {
        if (0 == rearrangeCounter_ && freeIDs_.size()) {
            auto rst = freeIDs_.back();
            freeIDs_.pop_back();
            return rst;
        } else {
            bufferAllocs_.emplace_back();
            return bufferAllocs_.size() - 1;
        }
    }

    mesh_buffer_alloc_t MeshBufferAllocator::deref(mesh_buffer_handle_t id) const {
        if(id < bufferAllocs_.size()) {
            return bufferAllocs_[id];
        }
        return {};
    }

    bool MeshBufferAllocator::free(mesh_buffer_handle_t id) {
        if(0 == rearrangeCounter_) {
            std::unique_lock<std::mutex> lock(mutex_);
            auto alloc = deref(id);
            if(alloc.buffer && bufferBlocks_[alloc.blockIndex].scheduler.free(alloc.offset)) {
                freeIDs_.push_back(id);
                bufferAllocs_[id] = {};
                return true;
            } else {
                assert(false);
                return false; // alloc is invalid
            }
        } else { // 重分配状态下不能立即回收，需要等待整理完再回收
            destroyRecords_.push_back(id);
            return true;
        }
    }

    struct buffer_cpy_t {
        VkBuffer dst;
        VkBuffer src;
        BufferSubResource dstRes;
        BufferSubResource srcRes;
    };
    static_assert(std::is_pod_v<buffer_cpy_t>, "");

    void MeshBufferAllocator::rearrangeBufferAlloc(ResourceCommandEncoder* encoder) {
        if(rearrangeCounter_) {
            return;
        }
        /**
         * @brief 先把原来所有的block备份到一个临时数组里，虽然接下来一帧或者两帧会用到，没有关系，我们暂时不删除它，比如，三缓冲模式，我们在两帧之后再删除，一定没问题。
         * 而我们合成后的buffer也是不能立即使用的！但是可以直接放`bufferBlocks_`里，没关系的。
         * 因为buffer更新在多队列里搞的话，需要非常严格的同步，所以这件事我们还是放渲染线程的queue里去做。
         * 这里会产生一些多重缓冲数据同步/有效性问题，所以这里的逻辑需要特别注意。
         */
        std::unique_lock<std::mutex> lock(mutex_);
        assert(rearrangingBlocks_.empty() && "must be empty");
        assert(rearrangingAllocs_.empty() && "must be empty");
        rearrangeCounter_ = MaxFlightCount;
        rearrangingBlocks_ = std::move(bufferBlocks_); // move blocks to temp vector
        std::vector<buffer_cpy_t> copies;
        uint32_t totalSize = 0;
        for(auto& alloc_t: bufferAllocs_) { // 原来的alloc保持不变
            if(alloc_t.buffer) {
                buffer_cpy_t cpy = { VK_NULL_HANDLE,alloc_t.buffer, BufferSubResource{totalSize, alloc_t.length}, BufferSubResource{alloc_t.offset, alloc_t.length} };
                copies.push_back(cpy); // buffer 后续再更新
                totalSize += alloc_t.length;
                mesh_buffer_alloc_t alloc = {VK_NULL_HANDLE, totalSize, alloc_t.length, 0, 1};
                rearrangingAllocs_.push_back(alloc); // buffer 后续更新
                totalSize = (totalSize+15)&(~15);
            } else {
                rearrangingAllocs_.emplace_back(); // 这里的alloc是空的
            }
        }
        auto rst = createNewBufferBlock_(totalSize);
        assert(rst);
        auto& newBlock = bufferBlocks_.back();
        // buffer barrier，因为buffer不会再使用了，所以不需要在传输完再加barrier了
        // 同理，新的buffer也不用加barrier同步，因为不需要，传输前传输后都不需要！想明白了吗?
        for(auto const& block: rearrangingBlocks_) {
            encoder->bufferBarrier(
                block.buffer, 
                ResourceAccessType::TransferSource, 
                PipelineStages::VertexShading, StageAccess::Read,
                PipelineStages::Transfer, StageAccess::Read,
                BufferSubResource{0, (uint32_t)block.bufferSize}
            );
        }
        // 更新相关数据
        for(auto& alloc: rearrangingAllocs_) { // allocation info
            if(alloc.length) {
                alloc.buffer = newBlock.buffer;
            }
        }
        // 写入复制指令等待送到队列执行
        for(auto& copy: copies) { // copy operation info
            copy.dst = newBlock.buffer;
            encoder->copyBuffer(copy.dst, copy.src, copy.dstRes, copy.srcRes);
        }
    }

    /**
     * @brief 主要是要处理 rearrange 的问题，期间删除buffer的时候，在onFrameTick里需要特别处理一些东西
     * 
     */
    void MeshBufferAllocator::onFrameTick() {
        if(rearrangeCounter_) { // deal rearrange stuffs
            --rearrangeCounter_;
            if(0 == rearrangeCounter_) {
                // 处理所有整理后的allocs
                for(size_t i = 0; i<rearrangingAllocs_.size(); ++i) {
                    auto& alloc = rearrangingAllocs_[i];
                    // 重新整理blockIndex
                    for(size_t blockIdx = 0; blockIdx<bufferBlocks_.size(); ++blockIdx) {
                        if(alloc.buffer && alloc.buffer == bufferBlocks_[blockIdx].buffer) {
                            alloc.blockIndex = blockIdx;
                        }
                    }
                }
                rearrangingAllocs_.clear();
                // 销毁所有 被排列过的 buffer
                for(auto const& block: rearrangingBlocks_) {
                    vkDestroyBuffer(device_->device(), block.buffer, nullptr); // destroy them
                }
                rearrangingBlocks_.clear();
                // 回收所有期间删除的buffer
                for(auto const& handle: destroyRecords_) {
                    this->free(handle);// destroy them
                }
            }
        }
    }

}