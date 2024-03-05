#pragma once
#include <core/declare.h>
#include <core/events/event_dispatcher.h>
#include <core/events/input_event.h>
#include <vector>
#include <functional>

namespace gui {

    using CaptureEventCallback = std::function<void(int)>;

    class InputHandler {
    private:
        std::vector<Touch*>     touches_;
        Component*              owner_;
        CaptureEventCallback    captureEventCallback_;
        InputEvent              recentInput_;
    public:
        static bool             touchOnUI;
        static uint32_t         touchOnUIFlagFrameID;
        static InputHandler*    activeHandler;
    public:
        InputEvent* recentInput() {
            return &recentInput_;
        }
        InputHandler(Component* owner);
        ~InputHandler();

        Point2D<int32_t> positionOfTouch(int touchID);
    };

}