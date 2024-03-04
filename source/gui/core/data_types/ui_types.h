#pragma once

#include "core/declare.h"
namespace gui {

    struct Margin {
        int left, right, top, bottom;
    };

    enum class FlipType {
        None,
        Hori,
        Vert,
        Both,
    };

    enum class FillMethod {
        None,
        Hori,
        Vert,
        Radial90,
        Radial180,
        Radial360,
    };

    enum class FillOrigin {
        None
    };

    enum class FillOriginHori {
        Left,
        Right
    };

    enum class FillOrginVert {
        Top,
        Bottom,
    };

    enum class FillOrigin90 {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    enum class FillOrigin180 {
        Top,
        Bottom,
        Left,
        Right,
    };

    enum class FillOrigin360 {
        Top,
        Bottom,
        Left,
        Right,
    };

    enum class ChildrenRenderOrder {
        Ascent,
        Descent,
        Arch,
    };

    enum class OverflowType {
        Visible,
        Hidden,
        Scroll
    };

}