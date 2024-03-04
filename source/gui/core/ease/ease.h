#pragma once

#include "ease_type.h"

namespace gui {

    namespace ease {

        float evaluate(EaseType type, float time, float duration, float overshootOrAmplitude, float period);

    }    

}