#include "UGITypes.h"
#include "UGIDeclare.h"

#include <vector>
#include <array>
#include <cstdint>
#include <cassert>
#include <functional>

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

    class UniformBuffer 
    {
    private:
        Buffer*     _buffer;
        uint32_t    _offset;
    public:
        UniformBuffer( Buffer* buffer, uint32_t offset );
        UniformBuffer( const UniformBuffer& buffer );
        void writeData( uint32_t offset, const void* data, uint32_t size );
        Buffer* buffer() {
            return _buffer;
        }
        uint32_t offset() {
            return _offset;
        }
    };
    class UniformAllocator 
    {
    private:
        Device*                                             _device;
        std::vector<Buffer*>                                _buffers;
        uint32_t                                            _offset;
        //
        std::array<std::vector<Buffer*>, MaxFlightCount+1>  _freeTable;
        uint32_t                                            _freeIndex;
        //
        uint32_t                                            _ringStart;
        uint32_t                                            _ringEnd;

        uint32_t                                            _minUniformAlignParam;
        struct FlightBufferRange { // begin - end 实际可能不是连续的
            uint32_t begin = 0;
            uint32_t end = 0;
            uint32_t size = 0;
        };
        // 这玩意是只有一个buffer时，为了循环使用ringbuffer用的，所以如果有分配时一个buffer不够导致分配多个buffer的情况时，这东西就应该重置了，不需要它了，因为下一个tick会重建buffer重来
        std::array<FlightBufferRange,MaxFlightCount>        _flightRanges;
        uint32_t                                            _flightIndexInside;
    public:
        UniformAllocator( Device* device );
        void tick();
        //
        UniformBuffer allocate( uint32_t size );

        template< class T >
        void allocateForDescriptor( ResourceDescriptor& descriptor, const T& value ) {
            assert( descriptor.type == ArgumentDescriptorType::UniformBuffer );
            auto ubo = allocate(sizeof(value));
            ubo.writeData(0, &value, sizeof(value) );
            descriptor.buffer = ubo.buffer();
            descriptor.bufferOffset = ubo.offset();
        }

        static UniformAllocator* createUniformAllocator( Device* device );
    };



}