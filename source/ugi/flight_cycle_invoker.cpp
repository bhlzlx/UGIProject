#include "flight_cycle_invoker.h"

namespace ugi {

    void FlightCycleInvoker::postCallable(std::function<void()>&& callable) {
        _invokeQueues[_flightIndex].push_back(std::move(callable));
    }

    void FlightCycleInvoker::tick() {
        auto& q = _invokeQueues[_flightIndex];
        for(auto& callable: q) {
            callable();
        }
        q.clear();
        ++_flightIndex;
        _flightIndex = _flightIndex % ( MaxFlightCount);
    }

    void FlightCycleInvoker::invokeAllNow() {
        for(auto i = 0; i<MaxFlightCount; ++i) {
            tick();
        }
    }
}