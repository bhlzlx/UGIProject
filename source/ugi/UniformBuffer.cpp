#include "UniformBuffer.h"
#include "Buffer.h"
#include "Device.h"
#include <cassert>

namespace ugi {


    constexpr uint32_t UniformAlignBytes = 256;
    constexpr uint32_t InitialBlockSize = 0x10000;
    //
    UniformAllocator::UniformAllocator( Device* device ) 
        : _device(device)
        , _alignSize(device->descriptor().properties.limits.minUniformBufferOffsetAlignment >= 8 ? device->descriptor().properties.limits.minUniformBufferOffsetAlignment - 1 : 7 )
        , _maxCapacity(InitialBlockSize)
        , _overflow(false)
        , _buffer{}
        , _ring(InitialBlockSize, _alignSize)
        , _freeTable{}
        , _flight(0)
    {
    }

    UniformAllocator::buf_t UniformAllocator::createUniformBlock(uint32_t size) {
        // size += 32; // 32 is what ever a number > 1
        auto vma = _device->vmaAllocator();
        VkBufferCreateInfo bufferInfo = {}; {
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.flags = 0;
            bufferInfo.pNext = nullptr;
            bufferInfo.pQueueFamilyIndices = _device->descriptor().queueFamilyIndices;
            bufferInfo.queueFamilyIndexCount = _device->descriptor().queueFamilyCount;
            if(_device->descriptor().queueFamilyCount>1 ) {
                bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            }else {
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            bufferInfo.size = size;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        }
        VmaAllocationCreateInfo allocationCreateInfo = {}; {
            allocationCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
        }
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkResult rst = vmaCreateBuffer(vma, &bufferInfo, &allocationCreateInfo, &buffer, &allocation, nullptr );
        buf_t buf {};
        buf.buf = buffer;
        buf.size = size;
        buf.vmaAlloc = allocation;
        vmaMapMemory(vma, allocation, (void**)&buf.ptr);
        assert(buf.ptr);
        return buf;
    }

    void UniformAllocator::tick() 
    {
        auto prevFlight = _flight;
        ++_flight;
        _flight = _flight % MaxFlightCount;
        if(_overflow) {
            _maxCapacity = _maxCapacity * 1.2;
            _maxCapacity = (_maxCapacity + _alignSize)&~(_alignSize);
            _freeTable[prevFlight].push_back(_buffer); // 回收上帧buffer
            _buffer = createUniformBlock(_maxCapacity);
            _ring.reset(_maxCapacity);
            _overflow = false;
        }
        _ring.prepareNextFlight();
        VmaAllocator vma = _device->vmaAllocator();
        for(auto& buf: _freeTable[_flight]) {
            vmaDestroyBuffer(vma, buf.buf, buf.vmaAlloc);
        }
    }

    uniform_t UniformAllocator::allocate(uint32_t size) {
        assert(size <= InitialBlockSize);
        uniform_t rst {};
        auto offset = _ring.alloc(size);
        if(offset == _ring.InvalidAlloc) { // 超出最大值了
            _freeTable[_flight].push_back(_buffer); // to free table, will be freed next cycle
            _buffer = createUniformBlock(InitialBlockSize);
            _maxCapacity += InitialBlockSize;
            _ring.reset(InitialBlockSize);
            _overflow = true;
            offset = _ring.alloc(size);
        }
        assert(offset != _ring.InvalidAlloc);
        rst.buffer = (size_t)_buffer.buf;
        rst.offset = offset;
        rst.ptr = _buffer.ptr + offset;
        rst.size = size;
        return rst;
    }

    UniformAllocator* UniformAllocator::createUniformAllocator(Device* device) {
        UniformAllocator* allocator = new UniformAllocator(device);
        ///> 我们需要注意的是 ringbuffer 必须是 2 的n次方且是 uniform buffer 最小对齐大小的倍数
        allocator->_buffer = allocator->createUniformBlock(InitialBlockSize);
        allocator->_ring.reset(InitialBlockSize);
        return allocator;
    }

}