#pragma once
#include "UGIDeclare.h"
#include "Resource.h"
#include "UGITypes.h"
#include <queue>
#include <array>
#include <functional>

namespace ugi {

    // class ResourceManager {
    // private:
    //     Device* _device;
    //     std::array<std::queue<Resource*>, MaxFlightCount+1>         _destroyQueues;
    //     uint32_t                                                    _flightIndex;
    // public:
    //     ResourceManager( Device* _device )
    //         : _device( _device )
    //         , _destroyQueues {}
    //         , _flightIndex( 0 )
    //     {
    //     }

    //     void trackResource(Resource* _res);
    //     void tick();
    // };

    class FlightCycleInvoker {
    private:
        std::array<std::vector<std::function<void()>>, MaxFlightCount>       _invokeQueues;
        uint16_t                                                             _flightIndex;
    public:
        FlightCycleInvoker()
            : _invokeQueues()
            , _flightIndex(0)
        {}
        void postCallable(std::function<void()>&& callable);
        void tick();
        void invokeAllNow();
    };

}