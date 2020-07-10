#pragma once

#include "VulkanDeclare.h"
#include "UGITypeMapping.h"
#include "UGITypes.h"
#include "UGIDeclare.h"
#include <vk_mem_alloc.h>
#include <memory>
#include <cassert>
#include "Resource.h"

/* -
    这里有一个有意思的问题，vk_mem_alloc 已经帮我们把内存管理做好了，但是 vulkan 还有一个 buffer 对象的数据限制
    实际上这个限制一般不会超过那么多，所以不需要再针对buffer来管理了
    注意创建buffer 大体上需要提供两种信息
    1. 会作为什么类型使用
    2. 谁能访问它（CPU/GPU）,以及是不是持久映射
-*/

namespace ugi {

    VkBufferUsageFlags getVkBufferUsageFlags( BufferType _type );
    VmaMemoryUsage getVmaAllocationUsage( BufferType _type );
    inline VkBufferCreateFlags getVkBufferCreateFlags( BufferType _type ) {
        return 0;
        /*
            VK_BUFFER_CREATE_SPARSE_BINDING_BIT = 0x00000001,
            VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT = 0x00000002,
            VK_BUFFER_CREATE_SPARSE_ALIASED_BIT = 0x00000004,
            VK_BUFFER_CREATE_PROTECTED_BIT = 0x00000008,
            VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT = 0x00000010,
            VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_EXT = VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT,
            VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_KHR = VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT,
        */
    }

    // ResourceAccessType accessTypeFromImageType( BufferType _type ) {
// 
    // }


    class Buffer : public Resource {
    private:
        BufferType              _bufferType;            // high level buffer 类型
        VkBuffer                _buffer;                // buffer 对象
        VmaAllocation           _allocation;            // buffer 的内存句柄
        uint32_t                _size;                     //
        //
        ResourceAccessType      _primaryAccessType;     // 资源创建意图的用途
        ResourceAccessType      _currentAccessType;     // 资源当前的用途
        //
        VkBool32                _mappable;
        void*                   _mappedPointer;
    public:

        Buffer() {
        }

        Buffer( BufferType type, VkBuffer handle, uint32_t size, VmaAllocation allocation ) 
            : _bufferType( type )
            , _buffer( handle )
            , _allocation( allocation )
            , _size(size)
            , _primaryAccessType( accessTypeFromBufferType(type) )
            , _currentAccessType( ResourceAccessType::None )
        {
        }

        bool mappable() const {
            return _mappable;
        }
        bool persisstentMappable() const {
            if( _bufferType == BufferType::VertexBufferStream ) {
                return true;
            }
            return false;
        }
        void* map( Device* _device );
        void unmap( Device* _device );
        void* pointer() {
            return _mappedPointer;
        }
        operator VkBuffer() const {
            return _buffer;
        }
        VkBuffer buffer() const {
            return _buffer;
        }
        ResourceAccessType primaryAccessType() {
            return _primaryAccessType;
        }
        ResourceAccessType accessType() {
            return _currentAccessType;
        }
        void updateAccessType( ResourceAccessType type ) {
            _currentAccessType = type;
        }
        uint32_t size() {
            return _size;
        }

        virtual void release( Device* device ) override;
        //
        static Buffer* CreateBuffer( Device* device, BufferType type, size_t size );
    };
}