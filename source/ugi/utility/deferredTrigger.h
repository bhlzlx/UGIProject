#pragma once
#include <cstdint>
#include <vector>
#include <array>

namespace ugi {

    template< class Type, class Handler, uint32_t FrameCount>
    class DeferredTrigger {
    private:
        std::array< std::vector<T>, FrameCount >    _queueArray;
        uint32_t                                    _frameIndex;
    public:
        DeferredTrigger()
            : _queueArray()
            , _frameIndex(0)
        {
        }

        void tick() {
            Handler handler;
            for( T& res : _queueArray[_frameIndex]) {
                handler(res);
            }
            _queueArray[_frameIndex].clear();
            //
            ++_frameIndex;
            _frameIndex = _frameIndex % FrameCount;
        }

        template< class N = T >
        void post( N&& res ) {
            uint32_t queueIndex = _frameIndex + FrameCount - 1;
            queueIndex = queueIndex % FrameCount;
            _queueArray[queueIndex].push_back(std::forward<N>(res));
        }
    };

}