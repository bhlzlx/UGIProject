#pragma once
#include "glm/fwd.hpp"
#include <core/declare.h>
#include <LightWeightCommon/utils/enum_class_bits.h>
#include <LightWeightCommon/utils/handle.h>

namespace gui {

    using namespace comm;

    enum class KeyModifier {
        Ctrl,
        Shift,
        Alt,
    };

    class InputEvent {
        Handle                      target_;
        glm::vec2                   pos_;
        int                         touchID_;
        int                         clickCount_;
        int                         mouseWheelDelta_;
        comm::BitFlags<KeyModifier> keyModifiers_;
    };

}