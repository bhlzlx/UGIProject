#include "ResourceManager.h"

namespace ugi {

    // 不是线程安全的，建议每一个队列持有一个，因为队列也不是线程安全的
    void ResourceManager::destroy( Resource* _res ) {
        uint32_t index = (m_flightIndex + MaxFlightCount)%(MaxFlightCount+1);
        m_destroyQueues[index].push(_res);
    }

    // 每个队列，每帧调用一次
    void ResourceManager::tick() {
        ++m_flightIndex;
        m_flightIndex = m_flightIndex % ( MaxFlightCount + 1 );
        auto& q = m_destroyQueues[m_flightIndex];
        //
        while(!q.empty()) {
            auto res = q.front();
            q.pop();
            res->release(m_device);
        }
    }
}