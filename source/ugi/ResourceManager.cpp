#include "ResourceManager.h"

namespace ugi {

    // 不是线程安全的，建议每一个队列持有一个，因为队列也不是线程安全的
    void ResourceManager::trackResource( Resource* _res ) {
        uint32_t index = (_flightIndex + MaxFlightCount)%(MaxFlightCount+1);
        _destroyQueues[index].push(_res);
    }

    // 每个队列，每帧调用一次
    void ResourceManager::tick() {
        ++_flightIndex;
        _flightIndex = _flightIndex % ( MaxFlightCount + 1 );
        auto& q = _destroyQueues[_flightIndex];
        //
        while(!q.empty()) {
            auto res = q.front();
            q.pop();
            res->release(_device);
        }
    }
}