#include "Buffer.h"
#include "Device.h"
#include "VulkanFunctionDeclare.h"

namespace ugi {

    VkBufferUsageFlags getVkBufferUsageFlags( BufferType type ) {
        uint32_t flags = 0;
        switch( type ) {
            case BufferType::VertexBuffer: {
                flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; break; 
            }
            case BufferType::VertexBufferStream: {
                flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break; 
            }
            case BufferType::IndexBuffer: {
                flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            }
            case BufferType::UniformBuffer: {
                flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
                break;
            }
            case BufferType::StagingBuffer: {
                flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                break;
            }
            case BufferType::ShaderStorageBuffer: {
                flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
                break;
            }
            default: {
                assert( false && "unsupported buffer type!" );
            }
        }
        return flags;
    }

    VmaMemoryUsage getVmaAllocationUsage( BufferType type ) {
        VmaMemoryUsage flag = VmaMemoryUsage::VMA_MEMORY_USAGE_UNKNOWN;
        switch(type){
            case BufferType::VertexBuffer:
            case BufferType::IndexBuffer:
            case BufferType::TexelBuffer:
            case BufferType::ShaderStorageBuffer: {
                flag = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
                break;
            }
            case BufferType::UniformBuffer: {
                flag = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
                break;
            }
            case BufferType::VertexBufferStream:
            case BufferType::StagingBuffer:{
                flag = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
                break;
            }
            case BufferType::ReadBackBuffer: {
                flag = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU;
            }
            case BufferType::InvalidBuffer:
            default:
            break;
        }
        return flag;
    }

    Buffer* Buffer::CreateBuffer(  Device* device, BufferType type, size_t size ) {
        // VkDevice device = _device->device();
        VkBufferCreateInfo bufferInfo = {}; {
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.flags = getVkBufferCreateFlags(type);
            bufferInfo.pNext = nullptr;
            bufferInfo.pQueueFamilyIndices = device->descriptor().queueFamilyIndices;
            bufferInfo.queueFamilyIndexCount = device->descriptor().queueFamilyCount;
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.size = size;
            bufferInfo.usage = getVkBufferUsageFlags( type );
        }
        VmaAllocationCreateInfo allocationCreateInfo = {}; {
            allocationCreateInfo.usage = getVmaAllocationUsage(type);
        }
        //
        auto vmaAllocator = device->vmaAllocator();
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkResult rst = vmaCreateBuffer( vmaAllocator, &bufferInfo, &allocationCreateInfo, &buffer, &allocation, nullptr );
        if( rst == VK_SUCCESS){
            Buffer* object = new Buffer( type, buffer, size, allocation );
            return object;
        }
        return nullptr;
    }

    void* Buffer::map( Device* device  ) {
        auto allocator = device->vmaAllocator();
        vmaMapMemory(allocator, _allocation, &_mappedPointer);
        return _mappedPointer;
    }
    
    void Buffer::unmap(Device* device ) {
        auto allocator = device->vmaAllocator();
        vmaUnmapMemory(allocator, _allocation);
    }

    void Buffer::release( Device* device ) {
        if(_allocation) {
            if( _mappedPointer) {
                unmap(device);
            }
            if(_buffer) {
                vmaDestroyBuffer( device->vmaAllocator(), _buffer, _allocation );
                _buffer = VK_NULL_HANDLE;
                _allocation = VK_NULL_HANDLE;
            }
        }
        delete this;
    }

}