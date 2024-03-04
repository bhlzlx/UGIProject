#pragma once
#include "core/data_types/handle.h"
#include "glm/fwd.hpp"
#include <core/declare.h>
#include <cstdint>

#include <LightWeightCommon/utils/enum_class_bits.h>

namespace gui {

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