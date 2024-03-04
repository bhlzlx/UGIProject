#pragma once

namespace gui {

    enum class TouchEvent {
        TouchBegin,
        TouchMove,
        TouchEnd,
    };

    enum class MouseEvent {
        MouseDown,
        MouseMove,
        MouseUp,
    };

    enum class UIEvent {
        AddedToStage,
        RemoveFromStage,
    };


}