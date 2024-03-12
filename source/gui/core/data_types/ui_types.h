#pragma once

#include "core/declare.h"
namespace gui {

    struct Margin {
        int left, right, top, bottom;
    };

    enum class FlipType : uint8_t {
        None,
        Hori,
        Vert,
        Both,
    };

    enum class FillMethod : uint8_t {
        None,
        Hori,
        Vert,
        Radial90,
        Radial180,
        Radial360,
    };

    enum class FillOrigin : uint8_t {
        None
    };

    enum class FillOriginHori : uint8_t {
        Left,
        Right
    };

    enum class FillOrginVert : uint8_t {
        Top,
        Bottom,
    };

    enum class FillOrigin90 : uint8_t {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    enum class FillOrigin180 : uint8_t {
        Top,
        Bottom,
        Left,
        Right,
    };

    enum class FillOrigin360 : uint8_t {
        Top,
        Bottom,
        Left,
        Right,
    };

    enum class ChildrenRenderOrder : uint8_t {
        Ascent,
        Descent,
        Arch,
    };

    enum class OverflowType : uint8_t {
        Visible,
        Hidden,
        Scroll
    };

}