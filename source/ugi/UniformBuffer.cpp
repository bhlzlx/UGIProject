#include "UniformBuffer.h"
#include "Buffer.h"
#include "Device.h"
#include <cassert>

namespace ugi {


    constexpr uint32_t UniformAlignBytes = 256;
    //
    UniformAllocator::UniformAllocator( Device* device ) 
        : _device( device )
        , _buffers {}
        , _offset(0)
        , _freeTable {}
        , _freeIndex(0)
        , _ringStart(0)
        , _ringEnd(0)
        , _minUniformAlignParam( device->descriptor().properties.limits.minUniformBufferOffsetAlignment >= 8 ? device->descriptor().properties.limits.minUniformBufferOffsetAlignment - 1 : 7 )
        , _flightRanges {}
        , _flightIndexInside (0)
    {
    }

    void UniformAllocator::tick() 
    {
        ++_freeIndex;
        _freeIndex = _freeIndex % (MaxFlightCount+1);
        ++_flightIndexInside;
        _flightIndexInside = _flightIndexInside % MaxFlightCount;
        //
        if( _buffers.size() > 1 ) { // 上一帧因为大小不够用又多分配了buffer?
            // 1. 清空 2. 分配一个1.5倍问题大小的buffer
            // 解释一下为什么清空就行了，因为们还有逻辑去处理销毁，重分配时会把当前的buffer引用投递到回收列表里，但是这个回收列表我们不会让它立即回收，而是会隔2~3帧回收
            uint32_t sizeTotal = 0;
            for( auto& buf : _buffers ) {
                sizeTotal += buf->size();
            }
            _buffers.clear();
            auto buf = _device->createBuffer( BufferType::UniformBuffer, sizeTotal );
            buf->map(_device);
            _ringStart = 0;
            _ringEnd = sizeTotal;
            _buffers.push_back( buf );
            for( uint32_t i = 0; i<MaxFlightCount; ++i ) {
                _flightRanges[i].begin = _flightRanges[i].end = _flightRanges[i].size;
            }
        }
        // 有需要删除的buffer了，删除之！
        if( _freeTable[_freeIndex].size() ) {
            for( auto buf : _freeTable[_freeIndex]) {
                buf->release(_device);
            }
            _freeTable[_freeIndex].clear();
        }
        //
        if( _flightRanges[_flightIndexInside].size ) {
            _ringEnd = _flightRanges[_flightIndexInside].end;
        }
        _flightRanges[_flightIndexInside].size = 0;

        //
        auto nextFlight = (_flightIndexInside+1) % MaxFlightCount;
        _flightRanges[_flightIndexInside].end = _flightRanges[_flightIndexInside].begin = _flightRanges[nextFlight].end;
    }

    UniformBuffer UniformAllocator::allocate( uint32_t size) {

        Buffer* targetBuffer = nullptr;
        uint32_t targetOffset = 0;

        Buffer* newbuffer = nullptr;

        uint32_t alignedIncreament = (size+_minUniformAlignParam)&~_minUniformAlignParam;

        if(_buffers.size() == 1 ) {
            auto buffer = _buffers.back();
            FlightBufferRange ranges[2]; // 
            if( _ringEnd > _ringStart ) { // 可分配的内存应该是1块
                ranges[0].begin = _flightRanges[_flightIndexInside].end;
                ranges[0].end = _ringEnd;
                ranges[0].size = _ringEnd - _ringStart;
            } else { // end 比 start 小说明，这个ringBuffer 中间被占用了，两边没有占用，所以有两块内存可以使用
                ranges[0].begin = _flightRanges[_flightIndexInside].end;
                ranges[0].end = buffer->size();
                ranges[0].size = buffer->size() - _ringStart;
                ranges[1].begin = 0;
                ranges[1].end = _ringEnd;
                ranges[1].size = _ringEnd;
            }

            if( ranges[0].size >= size ) { // 分配没问题！
                targetBuffer = buffer;
                targetOffset = _flightRanges[_flightIndexInside].end;
                _flightRanges[_flightIndexInside].end += (size+_minUniformAlignParam)&~_minUniformAlignParam;
                // _flightRanges[_flightIndexInside].end %= buffer->size();
                _flightRanges[_flightIndexInside].size += alignedIncreament;
                _ringStart = _flightRanges[_flightIndexInside].end;
            } else if ( ranges[1].size >= size ) { // 从 buffer 开头分配
                targetBuffer = buffer;
                targetOffset = 0;
                _flightRanges[_flightIndexInside].end = alignedIncreament;
                _flightRanges[_flightIndexInside].size += alignedIncreament;
                
                _ringStart = _flightRanges[_flightIndexInside].end;
            } else { // 还没法分配？？？ 那只能创建新buffer了
                newbuffer = _device->createBuffer( BufferType::UniformBuffer, buffer->size() );
                newbuffer->map( _device);
                _buffers.push_back(newbuffer);
                assert( buffer->size() >= size );
                _offset = alignedIncreament;
                targetBuffer = newbuffer;
                targetOffset = 0;
            }
        } else { //  数组里有多个buffer这种情况完全不需要考虑 ringbuffer 环路重用，不够就继续分配
            auto buffer = _buffers.back();
            if( buffer->size() - _offset >= size ) {
                targetBuffer = buffer;
                targetOffset = _offset;
                _offset += alignedIncreament;
            } else {
                newbuffer = _device->createBuffer( BufferType::UniformBuffer, buffer->size() * 2 );
                newbuffer->map( _device);
                _buffers.push_back(newbuffer);
                assert( buffer->size() >= size );
                _offset = alignedIncreament;
                targetBuffer = newbuffer;
                targetOffset = 0;
            }
        }
        if( newbuffer ) {
            _freeTable[(_freeIndex + MaxFlightCount) % (MaxFlightCount+1)].push_back( newbuffer );
        }
        return UniformBuffer(targetBuffer, targetOffset);
    }

    UniformAllocator* UniformAllocator::createUniformAllocator( Device* device ) {
        UniformAllocator* allocator = new UniformAllocator( device );
        ///> 我们需要注意的是 ringbuffer 必须是 2 的n次方且是 uniform buffer 最小对齐大小的倍数
        auto newbuffer = device->createBuffer( BufferType::UniformBuffer, 0x10000 );
        newbuffer->map(device);
        allocator->_buffers.push_back(newbuffer);
        allocator->_ringStart = 0;
        allocator->_ringEnd = 0x10000;
        return allocator;
    }

    UniformBuffer::UniformBuffer(Buffer* buffer, uint32_t offset ) 
        : _buffer( buffer )
        , _offset( offset )
    {}

    UniformBuffer::UniformBuffer( const UniformBuffer& buffer )
        : _buffer( buffer._buffer )
        , _offset( buffer._offset )
    {
    }

    void UniformBuffer::writeData( uint32_t offset, const void* data, uint32_t size ) {
        auto ptr = _buffer->pointer();
        uint8_t* bytes = (uint8_t*)(ptr) + offset + _offset;
        memcpy( bytes , data, size );
    }

}