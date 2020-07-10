#include "VulkanDeclare.h"
#include <vector>
#include <set>
#include <unordered_map>
#include <memory>


namespace ugi {

    enum class BufferType {
        InvalidBufferType             = 0,
        VertexBufferType             = 1,
        IndexBufferType                = 2,
        ShaderStorageBufferType     = 3,
        UniformBufferType            = 4,
        UniformTexelBufferType        = 5,
        StagingBufferType            = 6,
        ReadBackBufferType            = 7
    };

    // 功能：分配、回收内存，获取分配数据和状态
    // 
    class VulkanMemoryManager {
    public:
        enum class TypicalMemoryTypes {
            DeviceLocal                     = 0,        // typical used for : vertex buffer / texture / SSBO
            DeviceLocalLazyAllocate            = 1,        // typical used for : optimized GBuffer
            HostVisibleCached                = 2,        // typical used for : staging buffer
            HostVisiblePersistentMapping     = 3,        // typical used for : dynamic vertex buffers that should be update for every frame
            HostVisible                        = 4,        // typical used for : read back buffer
        };
    private:
        //
        struct MemoryInfomation {
            uint32_t                         memoryTypeIndex;
            uint32_t                         memoryHeapIndex;
            uint64_t                         size;
        };
    private:
        struct MemoryHeapInfo {
            size_t capacity;
            size_t used;
            VkMemoryHeapFlags flags;
        };
    private: // data members
        VkPhysicalDevice                                                m_physicalDevice;
        VkDevice                                                        m_device;
        VkPhysicalDeviceMemoryProperties                                 m_deviceMemoryProperties;
        std::vector<MemoryHeapInfo>                                     m_heaps;
        // allocation record
        std::unordered_map< VkDeviceMemory, MemoryInfomation >            m_allocedMemoryRecord;
    private: // functions
        // 把 `易用类型` 转换成 Vulkan 内部类型
        VkMemoryPropertyFlags convertMemoryPropertyFlagBits_( TypicalMemoryTypes _memoryType );
        // 获取 `内存类型`
        uint32_t queryMemoryTypeIndex_( const VkMemoryRequirements& _requirements, VkMemoryPropertyFlags _flagBits );
        // 获取 `内存类型` 对应的 `堆索引`
        uint32_t heapIndex_( uint32_t _memoryType ) { return m_deviceMemoryProperties.memoryTypes[_memoryType].heapIndex; }
        // 更新分配状态
        void updateAllocationInformation_( VkDeviceMemory _memory, const MemoryInfomation& _allocation );
        // 更新回收状态
        void updateDeallocationInformation_( const std::unordered_map< VkDeviceMemory, MemoryInfomation >::iterator& iter );
    public:
        VulkanMemoryManager()
            : m_physicalDevice( nullptr )
            , m_device( nullptr )
            , m_deviceMemoryProperties {}
            , m_heaps {}
            , m_allocedMemoryRecord {}
        {
        }
        // 初始化
        void initialize( VkPhysicalDevice _physicalDevice, VkDevice _device );
        // 分配
        VkDeviceMemory allocate( size_t size, const VkMemoryRequirements& _requirements, TypicalMemoryTypes _memoryType );
        // 回收
        void free( VkDeviceMemory _memory );
    };

    class Buffer {
    private:
        VkBuffer             m_buffer;
        VkDeviceMemory        m_memory;
        BufferType             m_bufferType;
        //
    public:
        Buffer( Buffer&& _buffer ) {
            m_buffer = _buffer.m_buffer;
            m_memory = _buffer.m_memory;
            m_bufferType = _buffer.m_bufferType;
            //
            _buffer.m_buffer = nullptr;
            _buffer.m_memory = nullptr;
            _buffer.m_bufferType = BufferType::InvalidBufferType;
        }
        Buffer()
            : m_buffer( nullptr )
            , m_memory( nullptr )
            , m_bufferType( BufferType::InvalidBufferType )
        {
        }

        static std::shared_ptr<Buffer> createBuffer( VkDevice _device, BufferType _type, size_t _size );

    };

}