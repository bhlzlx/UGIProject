#pragma once
#include <cstdint>

namespace ugi {

    /*
     * APHash 是一个哈希算法，从网上抄来的
     * */

    class APHash {
    private:
    public:
        uint64_t hash( uint64_t value, const void* data, size_t bytes ) {
            for ( size_t i = 0; i< bytes; ++i ) {
                uint8_t v = *((const uint8_t*)data + i);
                if (i & 1) {
                    value ^= ((value << 7) ^ v * (value >> 3));
                }
                else {
                    value ^= ((value << 11) ^ v * (value >> 5));
                }
            }
            return value;
        }
    };

    template< typename HashType = ugi::APHash >
    class UGIHash {
    private:
        uint64_t        _value;
        HashType        _hasher;
    public:
        UGIHash( uint64_t value = 0xAAAAAAAA )
            : _value( value )
        {
        }
        operator uint64_t () const {
            return _value;
        }
        
        template<class T>
        void hashPOD( const T& data ) {
            _value = _hasher.hash( _value, &data, sizeof(data) );
        }
        void hashBuffer(const void* buffer, size_t size ) {
            _value = _hasher.hash( _value, buffer, size );
        }
    };

}