#pragma once
#include "ugi_declare.h"
#include "resource.h"
#include "ugi_types.h"
#include <queue>
#include <array>
#include <functional>

namespace ugi {

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