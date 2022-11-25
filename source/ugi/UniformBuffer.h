#include "UGITypes.h"
#include "UGIDeclare.h"
#include "VulkanDeclare.h"
#include "UGIDeclare.h"

#include <vector>
#include <array>
#include <cstdint>
#include <cassert>
#include <functional>

#include <LightWeightCommon/memory/flight_ring.h>
#include <vk_mem_alloc.h>

namespace ugi {

    /*
     *  接下来我们来做一个uniform buffer分配器，分配uniform buffer
     *  假设，我们给一个 descriptor set 更新了 uniform，那么会发生两种情况
     *  1. 原来绑定的buffer对象没变，offset变了，这种代价最小
     *  2. 原来绑定的buffer变了，offset一般来说也变了 这种代价比较大，需要回收descriptor set然后再分配一个然后再写
     *  
     *  所以，如果一类descriptor set 用一个分配器去分配，能极大降低buffer对象切换的问题
     *  比如说　静态场景给一个uniform buffer allocator, 人物给一个，UI给一个，而uniform buffer通常需求量并不大，
     *  一般而言不会有不够得分配的过程，就算是有，我们重分配也能使绑定对对象不频繁切换
     *  所以这里引入一个 uniform allocator 的概念，一个 allocator 给一类资源使用
     * */

    struct uniform_t {
        size_t      buffer; 
        uint32_t    offset;
        uint32_t    size;
        uint8_t*    ptr;
    };

    class UniformAllocator 
    {
    private:
        struct buf_t {
            VkBuffer buf;
            uint8_t* ptr;
            uint32_t size;
            VmaAllocation vmaAlloc;
        };
    private:
        Device*                                             _device;
        uint32_t                                            _alignSize;
        uint32_t                                            _maxCapacity;
        bool                                                _overflow;
        buf_t                                               _buffer;
        comm::FlightRing<MaxFlightCount>                    _ring;
        //
        std::array<std::vector<buf_t>, MaxFlightCount>      _freeTable;
        uint32_t                                            _flight;

        buf_t createUniformBlock(uint32_t size);
    public:
        UniformAllocator(Device* device);
        void tick();
        uniform_t allocate(uint32_t size);

        // template< class T >
        // void allocateForDescriptor(res_descriptor_t& descriptor, const T& value) {
        //     assert( descriptor.type == res_descriptor_type::UniformBuffer );
        //     auto ubo = allocate(sizeof(value));
        //     memcpy(ubo.ptr, &value, descriptor.res.buffer.size);
        //     descriptor.res.buffer.buffer = ubo.buffer;
        //     descriptor.res.buffer.offset = ubo.offset;
        // }

        void allocateForDescriptor(res_descriptor_t& descriptor, void* ptr) {
            assert( descriptor.type == res_descriptor_type::UniformBuffer );
            auto ubo = allocate(descriptor.res.buffer.size);
            memcpy(ubo.ptr, ptr, descriptor.res.buffer.size);
            descriptor.res.buffer.buffer = ubo.buffer;
            descriptor.res.buffer.offset = ubo.offset;
        }

        // template< class T >
        // void allocateForDescriptor(res_descriptor_t& descriptor, const std::vector<T>& value) {
        //     assert( descriptor.type == res_descriptor_type::UniformBuffer );
        //     uint32_t uboSize = (uint32_t)(value.size() * sizeof(T));
        //     auto ubo = allocate( uboSize );
        //     ubo.writeData(0, value.data(), uboSize );
        //     descriptor.buffer = ubo.buffer();
        //     descriptor.bufferOffset = ubo.offset();
        // }

        static UniformAllocator* createUniformAllocator( Device* device );
    };



}