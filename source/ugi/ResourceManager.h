#pragma once
#include "UGIDeclare.h"
#include "Resource.h"
#include "UGITypes.h"
#include <queue>
#include <array>

namespace ugi {

    class ResourceManager {
    private:
        Device* m_device;
        std::array< std::queue<Resource*>, MaxFlightCount+1 >       m_destroyQueues;
        uint32_t                                                    m_flightIndex;
    public:
        ResourceManager( Device* _device )
            : m_device( _device )
            , m_destroyQueues {}
            , m_flightIndex( 0 )
        {
        }

        void trackResource( Resource* _res );
        void tick();
    };

}