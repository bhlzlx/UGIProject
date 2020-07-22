#include "Argument.h"
#include "Buffer.h"
#include "Texture.h"
#include "Descriptor.h"
#include "Device.h"
#include "ArgumentGroupLayout.inl"

namespace ugi {

    ArgumentGroupLayout::ArgumentGroupLayout()
        : _device( nullptr )
        , _descriptorSetCount(0)
        , _descriptorSetBitMask {}
        , _descriptorSetLayouts {}
        , _descriptorCountTotal(0)
        , _descriptorBindingMasks {}
        , _descriptorCount {}
        , _descriptorSetWriteBaseIndex {}
        , _dynamicBufferCountTotal(0)
        , _dynamicBufferCount {}
        , _dynamicOffsetBaseIndex {}
        , _imageCountTotal(0)
        , _imageDescriptorType {}
    {
    }

    bool ArgumentGroupLayout::validateResourceIntegrility( uint32_t _mask[] ) const {
        int rst = memcmp( _descriptorBindingMasks, _mask, sizeof(_descriptorBindingMasks));
        if( rst == 0 ) {
            return true;
        }
        return false;
    }
    
}