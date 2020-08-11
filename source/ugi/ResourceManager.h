#pragma once
#include "UGIDeclare.h"
#include "Resource.h"
#include "UGITypes.h"
#include <queue>
#include <array>

namespace ugi {

    class ResourceManager {
    private:
        Device* _device;
        std::array< std::queue<Resource*>, MaxFlightCount+1 >       _destroyQueues;
        uint32_t                                                    _flightIndex;
    public:
        ResourceManager( Device* _device )
            : _device( _device )
            , _destroyQueues {}
            , _flightIndex( 0 )
        {
        }

        void trackResource( Resource* _res );
        void tick();
    };

}