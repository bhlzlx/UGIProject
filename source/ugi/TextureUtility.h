#pragma once

#include "UGITypes.h"
#include "UGIDeclare.h"
#include <cstdint>

namespace ugi {

    class TextureUtility {
    private:
        Device*         _device;
        CommandQueue*   _transferQueue;
    public:
        TextureUtility( Device* device, CommandQueue* commandQueue )
            : _device(device)
            , _transferQueue(commandQueue)
        {
        }

        void replaceTexture( 
            Texture* texture, 
            const ImageRegion* regions,
            const void* data,
            uint32_t dataLength,
            uint32_t* offsets,
            uint32_t regionCount
        ) const;

        void replaceTexture( 
            Texture* texture, 
            const ImageRegion* regions,
            Buffer* stagingBuffer,
            uint32_t* offsets,
            uint32_t regionCount
        ) const;

        Texture* createTextureKTX(const void* data, uint32_t length) const;
        Texture* createTexturePNG(const void* data, uint32_t length) const;
        void generateMipmaps( Texture* texture ) const;
    };

}