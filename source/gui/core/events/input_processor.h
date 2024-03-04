#pragma once

#include "core/data_types/handle.h"
#include "core/declare.h"
#include "core/events/input_event.h"
#include "glm/glm.hpp"
#include <chrono>
#include <cstdint>
#include "../data_types/handle.h"
#include "utils/enum_class_bits.h"

namespace gui {

    class GTextInput;

    class TouchInfo {
    private:
    public:
        TouchInfo() {}
        ~TouchInfo() {}
        void reset();
        glm::vec2       pos;
        int             touchID;
        uint8_t         began:1;
        uint8_t         clickCancel:1;
        std::chrono::system_clock::time_point lastClickTime;
        Handle          lastRollover;
        int             clickCount;
        int             mouseWheelDelta;
        MouseButton     mouseButton;
    };

    class InputProcessor {
    private:
        InputEvent*                     lastInput_;
        std::vector<TouchInfo*>         touches_;
        Handle                          owner_;
        comm::BitFlags<KeyModifier>     keyModifiers_;
        GTextInput*                     activeInput_;
    public:
    };

}