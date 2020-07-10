#include "Buffer.h"
#include "VulkanFunctionDeclare.h"
#include <cassert>

// 这里我们不用 AMD 的内存管理了，为了更深入理解 Vulkan 的内存管理，这里我们自己实现一套显存管理方案

namespace ugi {

    /*
    typedef struct VkMemoryType {
        VkMemoryPropertyFlags    propertyFlags;
        uint32_t                 heapIndex;
    } VkMemoryType;

    typedef struct VkMemoryHeap {
        VkDeviceSize         size;
        VkMemoryHeapFlags    flags;
    } VkMemoryHeap;

    typedef struct VkPhysicalDeviceMemoryProperties {
        uint32_t        memoryTypeCount;
        VkMemoryType    memoryTypes[VK_MAX_MEMORY_TYPES];
        uint32_t        memoryHeapCount;
        VkMemoryHeap    memoryHeaps[VK_MAX_MEMORY_HEAPS];
    } VkPhysicalDeviceMemoryProperties
    */

    void VulkanMemoryManager::initialize( VkPhysicalDevice _physicalDevice, VkDevice _device ) {
        assert( _physicalDevice && _device );
        m_physicalDevice = _physicalDevice;
        m_device = _device;
        vkGetPhysicalDeviceMemoryProperties( m_physicalDevice, &m_deviceMemoryProperties );
        //
        this->m_heaps.resize(m_deviceMemoryProperties.memoryHeapCount);
    }

    uint32_t VulkanMemoryManager::queryMemoryTypeIndex_( const VkMemoryRequirements& _requirements, VkMemoryPropertyFlags _requiredMemoryProperyFlags ) {
        const uint32_t memoryTypeCount = m_deviceMemoryProperties.memoryTypeCount;
        for (uint32_t memoryIndex = 0; memoryIndex < memoryTypeCount; ++memoryIndex) {
            const uint32_t currentMemoryTypeBits = (1 << memoryIndex);
            uint32_t optionalMemoryTypeBits = _requirements.memoryTypeBits;
            const uint32_t typeBitsTestResult = currentMemoryTypeBits & optionalMemoryTypeBits;
            //
            const VkMemoryPropertyFlags properties = m_deviceMemoryProperties.memoryTypes[memoryIndex].propertyFlags;
            const bool hasRequiredProperties = (properties & _requiredMemoryProperyFlags) == _requiredMemoryProperyFlags;
            //
            if ( typeBitsTestResult && hasRequiredProperties){
                return static_cast<int32_t>(memoryIndex);
            }
        }
        return ~0;
    }

    void VulkanMemoryManager::updateAllocationInformation_( VkDeviceMemory _memory, const MemoryInfomation& _info ) {
        m_allocedMemoryRecord.emplace(_memory, _info );
        m_heaps[_info.memoryHeapIndex].used += _info.size;
    }

    void VulkanMemoryManager::updateDeallocationInformation_( const std::unordered_map< VkDeviceMemory, MemoryInfomation >::iterator& iter ) {
        m_heaps[iter->second.memoryHeapIndex].used -= iter->second.size;
        m_allocedMemoryRecord.erase(iter);
    }

    VkMemoryPropertyFlags VulkanMemoryManager::convertMemoryPropertyFlagBits_( TypicalMemoryTypes _memoryType ) {
        VkMemoryPropertyFlags flagsBits = 0;
        switch( _memoryType ) {
            case TypicalMemoryTypes::DeviceLocal : {
                flagsBits = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            }
            case TypicalMemoryTypes::DeviceLocalLazyAllocate : {
                flagsBits = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
                break;
            }
            case TypicalMemoryTypes::HostVisibleCached : {
                flagsBits = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                break;
            }
            case TypicalMemoryTypes::HostVisiblePersistentMapping : {
                flagsBits = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            }
        }
        return flagsBits;
    }

    VkDeviceMemory VulkanMemoryManager::allocate( size_t _size, const VkMemoryRequirements& _requirements, TypicalMemoryTypes _memoryType ) {
        VkMemoryPropertyFlags flags = convertMemoryPropertyFlagBits_(_memoryType);
        uint32_t memoryTypeIndex = queryMemoryTypeIndex_( _requirements, flags );
        //
        VkMemoryAllocateInfo info = {}; {
            if( _requirements.alignment ) {
                info.allocationSize = ( _size + _requirements.alignment - 1 ) & ~(_requirements.alignment - 1);
            } else {
                info.allocationSize = _size;
            }
            info.memoryTypeIndex = memoryTypeIndex;
            info.pNext = nullptr;
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        }
        VkDeviceMemory memory;
        VkResult rst = vkAllocateMemory(m_device, &info, nullptr, &memory);
        if( rst == VK_SUCCESS ) {
            MemoryInfomation recordInfo;
            recordInfo.memoryTypeIndex = memoryTypeIndex;
            recordInfo.memoryHeapIndex = heapIndex_(memoryTypeIndex);
            recordInfo.size = info.allocationSize;
            updateAllocationInformation_(memory, recordInfo) ;
            return memory;
        }
        return nullptr;
    }

    void VulkanMemoryManager::free( VkDeviceMemory _memory ) {
        auto iter = m_allocedMemoryRecord.find(_memory);
        if( iter != m_allocedMemoryRecord.end()) {
            updateDeallocationInformation_(iter);
        } else {
            // should report this error!
            assert(false);
            return;
        }
    }

    int32_t queryMemoryTypeIndex_(
                        const VkPhysicalDeviceMemoryProperties* pMemoryProperties,
                        uint32_t memoryTypeBits,
                        VkMemoryPropertyFlags requiredProperties
        ) {
        const uint32_t memoryTypeCount = pMemoryProperties->memoryTypeCount;
        for (uint32_t memoryIndex = 0; memoryIndex < memoryTypeCount; ++memoryIndex) {
            const uint32_t currentMemoryTypeBits = (1 << memoryIndex);
            const uint32_t typeBitsTestResult = currentMemoryTypeBits & memoryTypeBits;
            //
            const VkMemoryPropertyFlags properties = pMemoryProperties->memoryTypes[memoryIndex].propertyFlags;
            const bool hasRequiredProperties = (properties & requiredProperties) == requiredProperties;
            //
            if ( typeBitsTestResult && hasRequiredProperties) {
                return static_cast<int32_t>(memoryIndex);
            }
        }
        return -1;
    }

    // buffer 的描述只有使用类型，大小，没有访问限制（CPU/GPU 读/写）的描述，但是在实际分配内存的时候，这些特性需要我们去提供
    // 相当于 allocateMemory( bufferRequirement, VkMemoryPropertyFlags(硬件可读写性) )

    std::shared_ptr<Buffer> Buffer::createBuffer( VkDevice _device, BufferType _type, size_t _size ) {
        VkDevice device;
        VkBuffer buffer;
        //
        VkBufferCreateInfo info = {}; {
            info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.size = 0;
            info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            info.sharingMode;
            info.queueFamilyIndexCount;
            info.pQueueFamilyIndices;
        };
        vkCreateBuffer( device, &info, nullptr, &buffer );
        VkMemoryRequirements memoryRequirements;
        /*
            VkDeviceSize    size;
            VkDeviceSize    alignment;
            uint32_t        memoryTypeBits;
        */
        vkGetBufferMemoryRequirements( device, buffer, &memoryRequirements );
        //
        VkMemoryAllocateInfo memoryAllocateInfo = {};{
            memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocateInfo.
            memoryRequirements.memoryTypeBits = 
            memoryRequirements.
            /*
                VkStructureType    sType;
                const void*        pNext;
                VkDeviceSize       allocationSize;
                uint32_t           memoryTypeIndex;
            */
        }
        vkAllocateMemory( device, )
    }
    void createBuffer() {

        VkDevice device;
        VkBuffer buffer;

        VkBufferCreateInfo info = {}; {
            info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.size = 0;
            info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            info.sharingMode;
            info.queueFamilyIndexCount;
            info.pQueueFamilyIndices;
        };
        vkCreateBuffer( device, &info, nullptr, &buffer );
        VkMemoryRequirements memoryRequirements;
        /*
            VkDeviceSize    size;
            VkDeviceSize    alignment;
            uint32_t        memoryTypeBits;
        */
        vkGetBufferMemoryRequirements( device, buffer, &memoryRequirements );
        //
        VkMemoryAllocateInfo memoryAllocateInfo = {};{
            memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocateInfo.
            memoryRequirements.memoryTypeBits = 
            memoryRequirements.
            /*
                VkStructureType    sType;
                const void*        pNext;
                VkDeviceSize       allocationSize;
                uint32_t           memoryTypeIndex;
            */
        }
        vkAllocateMemory( device, )
    }
}