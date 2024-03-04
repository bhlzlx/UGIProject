#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <core/declare.h>

namespace gui {

    struct Touch {
        int                                 touchId;
        int                                 clickCount;
        int                                 mouseWheelDelta;
        MouseButton                         button;
        Point2D<int32_t>                    pos;
        Point2D<int32_t>                    downPos;
        bool                                began;
        bool                                clickCancelled;
        uint64_t                            clickTime;
        std::weak_ptr<Object>               lastRollover;
        std::vector<std::weak_ptr<Object>>  downTargets;
        //std::vector<WeakPtr> touchMonitors;:
        void reset() {
            touchId = -1;
            clickCount = 0;
        }
    };

}