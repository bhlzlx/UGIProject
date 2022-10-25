#include "MeshBufferAllocator.h"
#include <Device.h>
#include <CommandBuffer.h>

namespace ugi {

    VkBufferUsageFlags getVkBufferUsageFlags(BufferType _type);
    VmaMemoryUsage getVmaAllocationUsage(BufferType _type);

    bool MeshBufferAllocator::createNewBufferBlock(uint32_t size) {
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
            bufferInfo.usage = getVkBufferUsageFlags(BufferType::VertexBuffer);
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
            bufferAllocs_.emplace_back();
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
        auto rst = createNewBufferBlock(poolSize);
        return rst;
    }

    mesh_buffer_handle_t MeshBufferAllocator::alloc(uint32_t size) {
        mesh_buffer_alloc_t alloc = {};
        uint16_t blockIndex = 0;
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
                break;
            }
        }
        if(!alloc.buffer) { // alloc failed, need a new pool
            bool rst = createNewBufferBlock(size);
            if(!rst) {
                return mesh_buffer_handle_t{ mesh_buffer_handle_t::INVALID_HANDLE };
            }
            auto offset = bufferBlocks_.back().scheduler.alloc(size);
            alloc =  {bufferBlocks_.back().buffer, offset, size};
            blockIndex = bufferBlocks_.size() - 1;
        }
        auto freeLoc = allocLoc(blockIndex);
        bufferAllocs_[blockIndex][freeLoc] = alloc;
        mesh_buffer_handle_t handle = {}; 
        handle.block = blockIndex;
        handle.index = freeLoc;
        return handle;
    }

    uint16_t MeshBufferAllocator::allocLoc(size_t blockIndex) {
        assert(bufferBlocks_.size() > blockIndex);
        if(freeIndices_[blockIndex].size()) {
            auto rst = freeIndices_[blockIndex].back();
            freeIndices_.pop_back();
            return rst;
        } else {
            bufferAllocs_[blockIndex].emplace_back();
            return bufferAllocs_[blockIndex].size() - 1;
        }
    }

    mesh_buffer_alloc_t MeshBufferAllocator::deref(mesh_buffer_handle_t buf) const {
        assert(buf.block < bufferBlocks_.size());
        assert(buf.index < bufferAllocs_[buf.block].size());
        if(bufferAllocs_.size() <= buf.block || bufferAllocs_[buf.block].size() <= buf.index) {
            return {};
        }
        mesh_buffer_alloc_t rst = bufferAllocs_[buf.block][buf.index];
        return rst;
    }

    bool MeshBufferAllocator::free(mesh_buffer_handle_t buf) {
        auto alloc = deref(buf);
        if(bufferBlocks_[buf.block].scheduler.free(alloc.offset)) {
            freeIndices_[buf.block].push_back(buf.index);
            return true;
        } else {
            return false;
        }
    }

}